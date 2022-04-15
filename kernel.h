#ifndef KERNEL_H
#define KERNEL_H
#include <stdint.h>

#define KB 1024
#define MB KB*KB
#define PAGE_SIZE 4*KB


typedef struct{
    uint32_t access_type:2; // [1:0] must be b01, it means page table base address. b00 is a translation fault
    uint32_t SBZ_1:1; // [2]
    uint32_t NS:1; // [3] TODO what's this?
    uint32_t SBZ_2:1; // [4]
    uint32_t domain:4; // [8:5] defines the user or manager domain
    uint32_t P:1; // [9] TODO look at this again
    uint32_t coarse_pt_address:22; // [31:10]
}first_lvl_entry;

/* Won't be used by software, only here to help visualize. */
typedef struct{
    uint32_t SBZ:2; // [1:0]
    uint32_t table_index: 8; // [9:2]
    uint32_t coarse_pt_address:22; // [31:10]
}second_lvl_address;

typedef struct{
    uint32_t access_type:2; // [1:0] must be b10, for 4k pages
    uint32_t B:1; // [2]
    uint32_t C:1; // [3]
    uint32_t AP_0:2; // [5:4]
    uint32_t AP_1:2; // [7:6]
    uint32_t AP_2:2; // [9:8]
    uint32_t AP_3:2; // [11:10]
    uint32_t small_page_address:20; // [31:12]
}second_lvl_entry;

typedef struct{
    uint32_t page_index:12; // [11:0]
    uint32_t page_base_address:20; // [31:12]
}small_page_physical_address;

#endif