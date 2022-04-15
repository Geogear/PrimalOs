#ifndef KERNEL_H
#define KERNEL_H

#include "registers.h"

/* PAGE TABLE STRUCTURES */

typedef struct{
    uint32_t access_type:2; // [1:0] must be b01, it means page table base address. b00 is a translation fault.
    uint32_t SBZ_1:1; // [2]
    uint32_t NS:1; // [3] secure or non-secure world. For all operating modes except secure monitor, b1 and b0 ->  means secure.
    uint32_t SBZ_2:1; // [4]
    uint32_t domain:4; // [8:5] defines the user or manager domain.
    uint32_t P:1; // [9] usage of this not supported by the processor.
    uint32_t coarse_pt_address:22; // [31:10]
}first_lvl_entry;

/* Won't be used by software, only here to help visualize. */
typedef struct{
    uint32_t SBZ:2; // [1:0]
    uint32_t table_index: 8; // [9:2]
    uint32_t coarse_pt_address:22; // [31:10]
}second_lvl_address;

typedef struct{
    uint32_t access_type:2; // [1:0] must be b10, for 4k pages.
    uint32_t CB:2; // [3:2] C(acheable) and B(ufferable) bits,
    // b00 -> strongly ordered, b01 -> Shared Device
    // b10 -> Outer and Inner Write-Through, No Allocate on Write
    // b11 -> Outer and Inner Write-Back, No Allocate on Write
    // for more info, processor manual 6-15.
    uint32_t AP_0:2; // [5:4] all AP_N bits must be zero.
    uint32_t AP_1:2; // [7:6]
    uint32_t AP_2:2; // [9:8]
    uint32_t AP_3:2; // [11:10]
    uint32_t small_page_address:20; // [31:12]
}second_lvl_entry;

typedef struct{
    uint32_t page_index:12; // [11:0]
    uint32_t page_base_address:20; // [31:12]
}small_page_physical_address;

typedef struct{
    uint32_t allocated:1; // [0], if a process is using this page.
    uint32_t dirty:1; // [1], 1 if a write operation is performed for this page.
    uint32_t accessed:1; // [2], 1 if a read operation is performed for this page.
    uint32_t available:1; // [3]
    uint32_t pid:8; // [11:4] process using the page, 0 for the operating system.
    uint32_t small_page_address:20; // [31:12]
}os_pt_entry;

#endif