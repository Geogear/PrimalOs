#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <kernel/atag.h>
#include <kernel/kernel.h>
#include <kernel/list.h>

#define PAGE_SIZE 4096
#define KERNEL_HEAP_SIZE (1024*1024)
#define USER_HEAP_SIZE (PAGE_SIZE*4)
#define KERNEL_STACK_SIZE PAGE_SIZE
#define IRQ_STACK_SIZE PAGE_SIZE

typedef struct heap_segment{
    struct heap_segment * next;
    struct heap_segment * prev;
    uint32_t is_allocated;
    uint32_t segment_size;
} heap_segment_t;

uint32_t get_num_p(void);
uint32_t get_needed_page_count(uint32_t total_bytes);
os_pt_entry* get_pt_entry(uint32_t index);

void mem_init(atag_t* atags);

/* Returns the address of the pg entry, NULL on failure. */
os_pt_entry* page_alloc(uint32_t pid);

/* Returns -1 on failure, 0 on success. */
uint32_t page_free(uint32_t index);

/* Returns the pointer to the alloced mem segment, NULL on failure. */
void* mem_alloc(uint32_t bytes);
void mem_free(void* ptr);

#endif