#include <kernel/mem.h>
#include <kernel/atag.h>
#include <common/stdlib.h>
#include <common/stdio.h>
#include <stdint.h>
#include <stddef.h>

/* Defined in linker, end of the kernel image. */
extern uint8_t __end;

static uint32_t num_pages;
static os_pt_entry* all_pages;

static const uint32_t first_lvl_num_entries = 1024;
static first_lvl_entry* first_lvls;

static const uint32_t second_lvl_num_entries = 256;
static second_lvl_entry* second_lvls;

static uint32_t kernel_end_addr;

uint32_t get_num_p(void)
{
    return num_pages;
}

os_pt_entry get_pt_entry(uint32_t index)
{
    if(index < num_pages)
        return all_pages[index];
    return all_pages[0];
}

void mem_init(atag_t* atags) {
    uint32_t mem_size,  page_array_len_bytes, kernel_pages, 
    all_kernel_pages, i;

    // Get the total number of pages
    mem_size = 536870912; // TODO, can't calculate memsize form atags. get_mem_size(atags);
    num_pages = mem_size / PAGE_SIZE;

    puts("mem: ");
    log_uint(mem_size, 'h');

    puts("num_p: ");
    log_uint(num_pages, 'h');

    // Allocate space for all those pages' metadata.  Start this block just after the kernel image is finished
    page_array_len_bytes = sizeof(os_pt_entry) * num_pages;

    puts("pt_len:");
    log_uint(page_array_len_bytes, 'h');

    // Set start for os pt.
    all_pages = (os_pt_entry*)&__end;
    bzero(all_pages, page_array_len_bytes);

    // Set start for first lvl table.
    first_lvls = (first_lvl_entry*)&all_pages[num_pages];
    bzero(first_lvls, first_lvl_num_entries*sizeof(first_lvl_entry));

    // Set start for second lvl table.
    second_lvls = (second_lvl_entry*)&first_lvls[first_lvl_num_entries];
    bzero(second_lvls, first_lvl_num_entries*second_lvl_num_entries*sizeof(second_lvl_entry));
    
    kernel_end_addr = (uint32_t)&second_lvls[first_lvl_num_entries*second_lvl_num_entries];
    //INITIALIZE_LIST(free_pages);

    // Iterate over all pages and mark them with the appropriate flags
    // Start with kernel pages
    kernel_pages = ((uint32_t)&__end) / PAGE_SIZE;

    puts("kernel pages:");
    log_uint(kernel_pages, 'h');

    all_kernel_pages = kernel_end_addr / PAGE_SIZE;
    puts("all_kernel_pages:");
    log_uint(all_kernel_pages, 'h');

    for (i = 0; i < all_kernel_pages; i++) {
        all_pages[i].small_page_index = i;    // Identity map the kernel pages
        all_pages[i].allocated = 1;
    }

    // Map the rest of the pages as unallocated, and add them to the free list
    /*for(; i < num_pages; i++){
        all_pages_array[i].flags.allocated = 0;
        append_page_list(&free_pages, &all_pages_array[i]);
    }*/

}

uint32_t alloc_page(uint32_t pid)
{

}

uint32_t free_page(uint32_t index)
{
    if(num_pages < index)
        return -1;
    
    /* Set all except the page index to zero. */
    all_pages[index].allocated =
    all_pages[index].dirty =
    all_pages[index].accessed =
    all_pages[index].available =
    all_pages[index].pid = 0;

    return 0;
}