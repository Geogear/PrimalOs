#include <kernel/process.h>
#include <kernel/mem.h>
#include <kernel/interrupts.h>
#include <kernel/timer.h>
#include <common/stdlib.h>
#include <kernel/spin_lock.h>
#include <kernel/mutex.h>
#include <kernel/kerio.h>

static uint32_t next_proc_num = 0;
#define NEW_PID next_proc_num++;

// 10 ms
extern uint32_t quanta = 10000;

extern uint8_t __end;
extern void context_switch(process_control_block_t * old, process_control_block_t * new);
extern int try_lock(int * lock_var);

IMPLEMENT_LIST(pcb);

pcb_list_t run_queue;
pcb_list_t all_proc_list;

process_control_block_t * current_process;

void schedule(void) {
    DISABLE_INTERRUPTS();
    process_control_block_t * new_thread, * old_thread;

    // If nothing on the run queue, the current process should just continue
    if (size_pcb_list(&run_queue) == 0)
        return;

    // Get the next thread to run.  For now we are using round-robin
    new_thread = pop_pcb_list(&run_queue);
    old_thread = current_process;
    current_process = new_thread;

    // Put the current thread back in the run queue
    append_pcb_list(&run_queue, old_thread);

    // Context Switch
    context_switch(old_thread, new_thread);
    ENABLE_INTERRUPTS();
}

void process_init(void) {
    process_control_block_t * main_pcb;
    INITIALIZE_LIST(run_queue);
    INITIALIZE_LIST(all_proc_list);

    // Allocate and initailize the block
    main_pcb = mem_alloc(sizeof(process_control_block_t), get_heap_head());
    main_pcb->heap_segment_list_head = get_heap_head();
    INITIALIZE_LIST(main_pcb->allocated_pages);
    main_pcb->stack_page = (void *)&__end;
    main_pcb->pid = NEW_PID;
    memcpy(main_pcb->proc_name, "Init", 5);

    // Add self to all process list.  It is already running, so dont add it to the run queue
    append_pcb_list(&all_proc_list, main_pcb);

    current_process = main_pcb;

    // Set the timer to go off after 10 ms
    timer_set(quanta);
}

static void reap(void) {
    DISABLE_INTERRUPTS();
    process_control_block_t * new_thread, * old_thread;

    // If nothing on the run queue, there is nothing to do now. just loop
    while (size_pcb_list(&run_queue) == 0);

    // Get the next thread to run. For now we are using round-robin
    new_thread = pop_pcb_list(&run_queue);
    old_thread = current_process;
    current_process = new_thread;

    // Free the resources used by the old process. Technically, we are using dangling pointers here,
    // but since interrupts are disabled and we only have one core, it should still be fine
    os_pt_entry* entry = old_thread->allocated_pages.head;
    for(uint32_t i = 0; i < old_thread->allocated_pages.size; ++i){
        page_free(NULL, entry->small_page_index);
        entry = entry->nextos_pt_e;
    }
    page_free(old_thread->stack_page, -1);
    mem_free(old_thread);

    // Context Switch
    context_switch(old_thread, new_thread);
    // TODO enable interrupts?
}

//TODO return thread pid?
void create_kernel_thread(thread_function_f thread_func, char * name, int name_len) {
    process_control_block_t * pcb;
    proc_saved_state_t * new_proc_state;

    // Allocate and initialize the pcb
    pcb = mem_alloc(sizeof(process_control_block_t), get_heap_head());
    pcb->pid = NEW_PID;
    pcb->heap_segment_list_head = get_heap_head();
    pcb->stack_page = page_alloc(pcb->pid);
    INITIALIZE_LIST(pcb->allocated_pages);
    memcpy(pcb->proc_name, name, MIN(name_len,19));
    pcb->proc_name[MIN(name_len,19)+1] = 0;

    // Get the location the stack pointer should be in when this is run
    new_proc_state = pcb->stack_page + PAGE_SIZE - sizeof(proc_saved_state_t);
    pcb->saved_state = new_proc_state;

    // Set up the stack that will be restored during a context switch
    bzero(new_proc_state, sizeof(proc_saved_state_t));
    new_proc_state->lr = (uint32_t)thread_func;     // lr is used as return address in context_switch
    new_proc_state->sp = (uint32_t)reap;            // When the thread function returns, this reaper routine will clean it up
    new_proc_state->cpsr = 0x13 | (8 << 1);         // Sets the thread up to run in supervisor mode with irqs only

    // add the thread to the lists
    append_pcb_list(&all_proc_list, pcb);
    append_pcb_list(&run_queue, pcb);
}

void spin_init(spin_lock_t * lock) {
    *lock = 1;
}

void spin_lock(spin_lock_t * lock) {
    while (!try_lock(lock));
}

void spin_unlock(spin_lock_t * lock) {
    *lock = 1;
}

void mutex_init(mutex_t * lock) {
    lock->lock = 1;
    lock->locker = 0;
    INITIALIZE_LIST(lock->wait_queue);
}

void mutex_lock(mutex_t * lock) {
    process_control_block_t * new_thread, * old_thread;
    // If you don't get the lock, take self off run queue and put on to mutex wait queue
    while (!try_lock(&lock->lock)) {

        // Get the next thread to run.  For now we are using round-robin
        DISABLE_INTERRUPTS();
        new_thread = pop_pcb_list(&run_queue);
        old_thread = current_process;
        current_process = new_thread;

        // Put the current thread back of this mutex's wait queue, not on the run queue
        append_pcb_list(&lock->wait_queue, old_thread);

        // Context Switch
        switch_to_thread(old_thread, new_thread);
        ENABLE_INTERRUPTS();
    }
    lock->locker = current_process;
}

void mutex_unlock(mutex_t * lock) {
    process_control_block_t * thread;
    lock->lock = 1;
    lock->locker = 0;

    // If there is anyone waiting on this, put them back in the run queue
    if (size_pcb_list(&lock->wait_queue)) {
        thread = pop_pcb_list(&lock->wait_queue);  
        push_pcb_list(&run_queue, thread);
    }
}