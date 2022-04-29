#include <kernel/mem.h>
#include <kernel/uart.h>
#include <common/stdlib.h>
#include <stddef.h>

// Defined in linker, end of the kernel image.
extern uint8_t __end;

// All physical pages.
static uint32_t num_pages;
static os_pt_entry* all_pages;

// First lvl page table for the vm.
static const uint32_t first_lvl_num_entries = 1024;
static first_lvl_entry* first_lvls;

// Second lvl page tables for the vm.
static const uint32_t second_lvl_num_entries = 256;
static second_lvl_entry* second_lvls;

// TODO Process table.
static const uint32_t proc_t_num_entries = 256;

// Generate a list type from the struct.
DEFINE_LIST(os_pt_e);
IMPLEMENT_LIST(os_pt_e);

// Compiler generated list type.
os_pt_e_list_t free_pages;
// Used for the heap only.
os_pt_e_list_t allocated_pages;

// Dynamic memory segment header.
static heap_segment_t* heap_segment_list_head;

// Kernel end with the page tables.
static uint32_t kernel_end_addr;

uint32_t get_num_p(void)
{
    return num_pages;
}

uint32_t get_needed_page_count(uint32_t total_bytes)
{
    if(total_bytes < PAGE_SIZE)
        return 1;
    if(total_bytes % PAGE_SIZE == 0)
        return total_bytes / PAGE_SIZE;
    return total_bytes / PAGE_SIZE + 1;
}

os_pt_entry* get_pt_entry(uint32_t index)
{
    if(num_pages <= index)
        return NULL;
    return &all_pages[index];
}

void mem_init(atag_t* atags) {
    uint32_t mem_size,  page_array_len_bytes, kernel_pages, 
    all_kernel_pages, i;

    // Get the total number of pages
    mem_size = 536870912; // TODO, can't calculate memsize form atags. get_mem_size(atags);
    num_pages = mem_size / PAGE_SIZE;

    // Allocate space for all those pages' metadata.  Start this block just after the kernel image is finished
    page_array_len_bytes = sizeof(os_pt_entry) * num_pages;

    // Set start for os pt.
    all_pages = (os_pt_entry*)&__end;
    bzero(all_pages, page_array_len_bytes);
    INITIALIZE_LIST(free_pages);
    INITIALIZE_LIST(allocated_pages);

    // Set start for first lvl table.
    first_lvls = (first_lvl_entry*)&all_pages[num_pages];
    bzero(first_lvls, first_lvl_num_entries*sizeof(first_lvl_entry));

    // Set start for second lvl table.
    second_lvls = (second_lvl_entry*)&first_lvls[first_lvl_num_entries];
    bzero(second_lvls, first_lvl_num_entries*second_lvl_num_entries*sizeof(second_lvl_entry));
    
    // Set kernel end with pts included.
    kernel_end_addr = (uint32_t)&second_lvls[first_lvl_num_entries*second_lvl_num_entries];

    // Iterate over all pages and mark them with the appropriate flags
    // Start with kernel pages
    kernel_pages = ((uint32_t)&__end) / PAGE_SIZE;

    // Calc needed heap pages amount.
    uint32_t heap_pages_num = get_needed_page_count(KERNEL_HEAP_SIZE);

    uart_puts("kernel pages:");
    log_uint(kernel_pages, 'h');

    all_kernel_pages = get_needed_page_count(kernel_end_addr);//kernel_end_addr / PAGE_SIZE;
    uart_puts("all_kernel_pages:");
    log_uint(all_kernel_pages, 'h');

    uart_puts("os_pt_entry size: ");
    log_uint(sizeof(os_pt_entry), 'h');

    uart_puts("user heap: ");
    log_uint(USER_HEAP_SIZE, 'h');

    uart_puts("heap header: ");
    log_uint(sizeof(heap_segment_t), 'h');

    uart_puts("heap page count: ");
    log_uint(heap_pages_num, 'h');

    // Identity map the kernel pages.
    for (i = 0; i < all_kernel_pages; i++) {
        all_pages[i].small_page_index = i;
        all_pages[i].allocated = 1;
    }

    // Init 1st lvl table.
    for(i = 0; i < first_lvl_num_entries; ++i)
    {
        // Means, points to a page table.
        first_lvls[i].access_type = 1;
        // Identity map for the kernel pages.
        uint32_t tmp = (uint32_t)&second_lvls[i*second_lvl_num_entries];
        // Only want the msb 22 bits.
        first_lvls[i].coarse_pt_address = tmp >> 10;
        // Leaving the domain as 0 for the kernel, meaning
        // Domain not changed, so domain 0 must be a manager domain.
    }

    // Init 2nd lvl table.
    for(i = 0; i < second_lvl_num_entries * first_lvl_num_entries; ++i)
    {
        // Means, points to a 4kb page
        second_lvls[i].access_type = 2;
        // Identity map the page.
        uint32_t tmp = (uint32_t)&all_pages[i];
        // Only want the msb 20 bits.
        second_lvls[i].small_page_address = tmp >> 12;
        // TODO cb, stays as strongly ordered for now.
    }

    // Init heap.
    heap_segment_list_head = (heap_segment_t *) kernel_end_addr;
    bzero(heap_segment_list_head, sizeof(heap_segment_t));
    heap_segment_list_head->segment_size = KERNEL_HEAP_SIZE; 
    // Identity map, heap pages as well.
    for(i = all_kernel_pages; i < all_kernel_pages + heap_pages_num; ++i)
    {
        all_pages[i].allocated = 1;
        all_pages[i].small_page_index = i;
        append_os_pt_e_list(&allocated_pages, &all_pages[i]);
    }  

    // Map the rest of the pages as unallocated, and add them to the free list
    for(i = all_kernel_pages + heap_pages_num; i < num_pages; ++i){
        all_pages[i].allocated = 0;
        append_os_pt_e_list(&free_pages, &all_pages[i]);
    }

}

os_pt_entry* page_alloc(uint32_t pid)
{
    os_pt_entry* entry;
    // If no free pages left or pid is invalid.
    if(size_os_pt_e_list(&free_pages) == 0 || proc_t_num_entries <= pid)
        return NULL;

    // Get a page and set properties.
    entry = pop_os_pt_e_list(&free_pages);
    entry->allocated = 1;
    entry->pid = pid;

    // Zero out the page memory.
    void* page_mem = (void*)(entry->small_page_index * PAGE_SIZE);
    bzero(page_mem, PAGE_SIZE);

    // IF you have the netry, you have the page memory.
    return entry;
}

uint32_t page_free(uint32_t index)
{
    if(num_pages <= index)
        return -1;
    
    // Set all except the page index to zero.
    all_pages[index].allocated =
    all_pages[index].dirty =
    all_pages[index].accessed =
    all_pages[index].available =
    all_pages[index].pid = 0;

    // Add it to the free list.
    append_os_pt_e_list(&free_pages, &all_pages[index]);

    return 0;
}

void* mem_alloc(uint32_t bytes)
{
    heap_segment_t* cur, *best = NULL;
    // Max signed int.
    int diff, best_diff = 0x7fffffff;

    // Add the header to alloc size and align it wşth 16 byte.
    uint32_t header_size = sizeof(heap_segment_t);
    bytes += header_size;
    bytes += bytes % 16 ? 16 - (bytes % 16) : 0;

    // Find the allocation that is closest in size to this request.
    for (cur = heap_segment_list_head; cur != NULL; cur = cur->next) {
        diff = cur->segment_size - bytes;
        if (!cur->is_allocated && diff < best_diff && diff >= 0) {
            best = cur;
            best_diff = diff;
        }
    }

    // If no free mem is available.
    // TODO, update so that a new segment acquisition is tried,
    // this would need pid when including users in the future.
    if(best == NULL)
        return NULL;

    // If best diff is larger than the threshold, split it into two segments.
    if (best_diff > (int)(2 * header_size)) {
        bzero(((void*)(best)) + bytes, header_size);
        cur = best->next;
        best->next = ((void*)(best)) + bytes;
        best->next->next = cur;
        best->next->prev = best;
        best->next->segment_size = best->segment_size - bytes;
        best->segment_size = bytes;
    }
    best->is_allocated = 1;
    return best + 1;
}

void mem_free(void* ptr) {
    heap_segment_t* seg;

    if (!ptr)
        return;

    seg = ptr - sizeof(heap_segment_t);
    seg->is_allocated = 0;

    // Merge with freed segements on the left.
    while(seg->prev != NULL && !seg->prev->is_allocated) {
        seg->prev->next = seg->next;
        seg->next->prev = seg->prev;
        seg->prev->segment_size += seg->segment_size;
        seg = seg->prev;
    }
    // Merge with freed segments on the right.
    while(seg->next != NULL && !seg->next->is_allocated) {
        seg->next->next->prev = seg;
        seg->next = seg->next->next;
        seg->segment_size += seg->next->segment_size;
        seg = seg->next;
    }
}