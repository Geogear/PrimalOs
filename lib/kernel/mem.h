#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <kernel/atag.h>
#include <kernel/kernel.h>
#include <kernel/list.h>

#define PAGE_SIZE 4096

uint32_t get_num_p(void);
uint32_t get_needed_page_count(uint32_t total_bytes);
os_pt_entry* get_pt_entry(uint32_t index);
void mem_init(atag_t* atags);

/* Returns the address of the pg entry, NULL on failure. */
os_pt_entry* alloc_page(uint32_t pid);

/* Returns -1 on failure, 0 on success. */
uint32_t free_page(uint32_t index);

#endif