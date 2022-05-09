#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <kernel/atag.h>
#include <kernel/kernel.h>
#include <kernel/list.h>

#define PAGE_SIZE 4096
#define KERNEL_HEAP_SIZE (1024*1024)
#define USER_HEAP_SIZE PAGE_SIZE
#define KERNEL_STACK_SIZE PAGE_SIZE

typedef struct heap_segment{
    struct heap_segment * next;
    struct heap_segment * prev;
    uint32_t is_allocated;
    uint32_t segment_size;
} heap_segment_t;


// Define a list type for page table entries.
DEFINE_LIST(os_pt_e);

typedef struct os_pt_e{
    uint32_t allocated:1; // [0], if a process is using this page.
    uint32_t dirty:1; // [1], 1 if a write operation is performed for this page.
    uint32_t accessed:1; // [2], 1 if a read operation is performed for this page.
    uint32_t available:4; // [3]
    uint32_t pid:8; // [11:4] process using the page, 0 for the operating system.
    uint32_t small_page_index:17; // [31:12]
    DEFINE_LINK(os_pt_e);
}os_pt_entry;

uint32_t get_num_p(void);
uint32_t get_needed_page_count(uint32_t total_bytes);
os_pt_entry* get_pt_entry(uint32_t index);
heap_segment_t* get_heap_head(void);

void mem_init(atag_t* atags);

/* Returns the address of the page, NULL on failure. */
void* page_alloc(uint32_t pid);
void page_free(void* ptr, uint32_t index);

/* Returns the pointer to the alloced mem segment, NULL on failure. */
void* mem_alloc(uint32_t bytes, heap_segment_t* segment_head);
void mem_free(void* ptr);

#endif