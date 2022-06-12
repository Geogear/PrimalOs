#include <kernel/list.h>
#include <kernel/mem.h>
#include <stddef.h>

#ifndef PROCESS_H
#define PROCESS_H

extern uint32_t quanta;

typedef void (*thread_function_f)(void);

typedef struct {
    uint32_t r0;
    uint32_t r1; 
    uint32_t r2; 
    uint32_t r3; 
    uint32_t r4; 
    uint32_t r5; 
    uint32_t r6; 
    uint32_t r7; 
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t cpsr; 
    uint32_t sp;
    uint32_t lr;
} proc_saved_state_t;

DEFINE_LIST(pcb);

typedef struct pcb {
    proc_saved_state_t * saved_state; // Pointer to where on the stack this process's state is saved. Becomes invalid once the process is running
    void * stack_page;                // The stack for this proces.  The stack starts at the end of this page
    uint32_t pid;                     // The process ID number
    uint32_t blocker_id;              // Blocker sem id
    DEFINE_LINK(pcb);
    char proc_name[20];               // The process's name
    os_pt_e_list_t allocated_pages;
    heap_segment_t* heap_segment_list_head;               
} process_control_block_t;

void process_init(void);

void create_kernel_thread(thread_function_f thread_func, char * name, int name_len);
void schedule(void);

#endif