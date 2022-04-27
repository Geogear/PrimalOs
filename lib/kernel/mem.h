#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <kernel/atag.h>
#include <kernel/kernel.h>

#define PAGE_SIZE 4096

uint32_t get_num_p(void);
os_pt_entry get_pt_entry(uint32_t index);
void mem_init(atag_t* atags);

/* Returns the index of allocated page, -1 on failure. */
uint32_t alloc_page(uint32_t pid);

/* Returns -1 on failure, 0 on success. */
uint32_t free_page(uint32_t index);

#endif