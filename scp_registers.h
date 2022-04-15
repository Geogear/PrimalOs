#ifndef SCPR_H
#define SCPR_H
#include <stdint.h>

#define KB 1024
#define MB KB*KB
#define PAGE_SIZE 4*KB

/* Used to set access rights for the coprocessors CP to CP13. */
typedef struct{
    // For any cp access:
    // b00 -> access denied, reset val.
    // b01 -> priviliged access only.
    // b10 -> reserved.
    // b11 -> priviliged and user access.
    uint32_t CP0:2; // [1:0]
    uint32_t CP1:2; // [3:2]
    uint32_t CP2:2; // [5:4]
    uint32_t CP3:2; // [7:6]
    uint32_t CP4:2; // [9:8]
    uint32_t CP5:2; // [11:10]
    uint32_t CP6:2; // [13:12]
    uint32_t CP7:2; // [15:14]
    uint32_t CP8:2; // [17:16]
    uint32_t CP9:2; // [19:18]
    uint32_t CP10:2; // [21:20]
    uint32_t CP11:2; // [23:22]
    uint32_t CP12:2; // [25:24]
    uint32_t CP13:2; // [27:26]
    uint32_t SBZ:4; // [31:28]
}c1_coprocessor_access_control_r;

/* This structure assumes n = 0, in the ttb control register.*/
typedef struct{
    uint32_t C:1; // [0] indicates pt walk is inner cacheable or not
    // b0 -> inner non-cacheable, reset val. b1-> inner cacheable.
    uint32_t S:1; // [1] indicates if pt walk is to non-shared or shared memory
    // b0 -> non-shared, reset val. b1-> shared
    uint32_t P:1; // [2] b0- > disables error-correctig-code, reset val.
    uint32_t RGN:2; // [4:3] outer cacheable attributes for page walking
    // b00 -> outer noncacheable, reset val.
    // b01 -> write-back, write allocate
    // b10 -> write-through, no allocate on write
    // b11 -> write-back, no allocate on write
    uint32_t SBZ:9; // [13:5]
    uint32_t translation_table_base0:18; // [31:14] address of the first level tt.
}c2_ttb_r0;

typedef struct{
    uint32_t N:3; // [2:0] boundary size of ttb_r0
    uint32_t SBZ1:1; // [3]
    uint32_t PD0:1; // [4] b0-> enables pt walk for ttbr_0, reset val.
    uint32_t PD1:1; // [5] b0-> enables pt walk for ttbr_1, reset val.
    uint32_t SBZ2:26; // [31:6]
}c2_ttb_control_r;

/* Holds access permissions for a maximum of 16 domains. */
typedef struct{
    // For any domain, defines access permissions.
    // b00 -> access denied, reset val. Generates a domain fault.
    // b01 -> client, acesses are checked against permssion bits in the tlb entry.
    // b10 -> reserved. Generates domain fault.
    // b11 -> manager, accesses are not checked against the access permission bits in the tlb entry.
    uint32_t D0:2; // [1:0]
    uint32_t D1:2; // [3:2]
    uint32_t D2:2; // [5:4]
    uint32_t D3:2; // [7:6]
    uint32_t D4:2; // [9:8]
    uint32_t D5:2; // [11:10]
    uint32_t D6:2; // [13:12]
    uint32_t D7:2; // [15:14]
    uint32_t D8:2; // [17:16]
    uint32_t D9:2; // [19:18]
    uint32_t D10:2; // [21:20]
    uint32_t D11:2; // [23:22]
    uint32_t D12:2; // [25:24]
    uint32_t D13:2; // [27:26]
    uint32_t D14:2; // [29:28]
    uint32_t D15:2; // [31:30]
}c3_domain_access_control_r;

/* Holds the source of the last data fault. */
typedef struct{
    // TODO 
}c5_data_fault_status_r;

/* Software must perform a Data Synchronization Barrier
operation before changes to the register. To ensure that
all accesses are done to the correct context id. */
typedef struct{
    uint32_t asid:8; // [7:0] basically procid but used by the hw in the tlb entries.
    uint32_t procid:24; // [31:8]
}c13_context_id_r;

#endif