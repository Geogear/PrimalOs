#include <kernel/process.h>
#include <kernel/mem.h>
#include <kernel/interrupts.h>
#include <kernel/timer.h>
#include <common/stdlib.h>
#include <kernel/spin_lock.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <kernel/kerio.h>

static uint32_t next_proc_num = 0;
extern uint32_t next_sem_id = 0;
#define NEW_PID next_proc_num++;

// 10 ms
extern uint32_t quanta = 10000;

extern uint8_t __end;
extern void context_switch(process_control_block_t * old, process_control_block_t * new);
extern int try_lock(int * lock_var);

IMPLEMENT_LIST(pcb);

pcb_list_t run_queue;
pcb_list_t all_proc_list;
pcb_list_t block_queue;

process_control_block_t * current_process;

static uint32_t sems_taken = 0;
static uint32_t sems_pending = 0;

static void block(uint32_t blocker_id){
    // Set the blocker id
    current_process->blocker_id = blocker_id;

    // Make the thread switch
    process_control_block_t * new_thread, * old_thread;
    DISABLE_INTERRUPTS();

    // Get the next thread to run.  For now we are using round-robin
    new_thread = pop_pcb_list(&run_queue);
    old_thread = current_process;
    current_process = new_thread;

    // Put the current thread back of the sem blocked queue, not on the run queue
    append_pcb_list(&block_queue, old_thread);

    // Context Switch
    context_switch(old_thread, new_thread);
    ENABLE_INTERRUPTS();       
}

static void wake_up(uint32_t waker_id){
    struct pcb* blocked_thread = block_queue.head;
    while(blocked_thread != NULL){
        // Remove the thread from blocked queue and put in to the run queue
        if(blocked_thread->blocker_id == waker_id){
            remove_pcb(&block_queue, blocked_thread);
            blocked_thread->blocker_id = -1;
            append_pcb_list(&run_queue, blocked_thread);
            // Clear the pending signal.
            sems_pending = sems_pending ^ (0x1 << waker_id);
            break;
        }
    }
}

static void check_blockeds(void){
    // If there are any signal waiting
    if(sems_pending != 0){
        // For each waiting signal, try to wake up
        for(uint32_t i = 0; i < TOTAL_SEMS; ++i){
            uint32_t pending = (sems_pending >> i) & 0x1;
            if(pending){
                wake_up(i);
            }
        }
    }
}

void schedule(void) {
    DISABLE_INTERRUPTS();
    // First check for pending signals on the blocked queue.
    check_blockeds();

    // Thread switch.
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
    INITIALIZE_LIST(block_queue);

    // Allocate and initailize the block
    main_pcb = mem_alloc(sizeof(process_control_block_t), get_heap_head());
    main_pcb->heap_segment_list_head = get_heap_head();
    INITIALIZE_LIST(main_pcb->allocated_pages);
    main_pcb->stack_page = (void *)&__end;
    main_pcb->pid = NEW_PID;
    main_pcb->blocker_id = -1;
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
    pcb->blocker_id = -1;
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
        context_switch(old_thread, new_thread);
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

int sem_init(sem_t* sem, int val){
    uint32_t found = 0;
    sem->val = val;
    for(uint32_t i = 0; i < TOTAL_SEMS; ++i){
        uint32_t bit = (sems_taken >> i) & 0x1;
        if(!bit){
            sems_taken = sems_taken | (0x1 << i);
            sem->id = i;
            found = 1;
            break;
        }
    }

    if(!found){
        printf("ERROR: ALL SYSTEM SEMAPHORES ARE TAKEN.\n");
        sem->id = -1;
        return -1;
    }
    return 0;
}

void sem_wait(sem_t* sem){
    sem->val -= 1;
    if(sem->val <= 0)
        block(sem->id);
}

void sem_signal(sem_t* sem){
    sem->val += 1;
    if(sem->val > 0)
        wake_up(sem->id);
}

void sem_destroy(sem_t* sem){
    sems_taken = sems_taken ^ (0x1 << sem->id);
}