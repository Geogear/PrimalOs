#ifndef SCPR_H
#define SCPR_H
#include <stdint.h>

#define KB 1024
#define MB KB*KB
#define PAGE_SIZE 4*KB

/* SYSTEM CONTROL COPROCESSOR REGISTERS */

typedef struct{
    uint32_t M:0; // [0] enables the mmu.
    uint32_t A:1; // [1] enables strict alignment of data to detect alignment faults in data accesses.
    uint32_t C:1; // [2] enables lvl 1 data cache.
    uint32_t W:1; // [3] not implemented.
    uint32_t SBO:3; // [6:4] should be 1
    uint32_t B:1; // [7] determines endienness. b0 -> little endian, reset val.
    uint32_t S:1; // [8] deprecated
    uint32_t R:1; // [9] deprecated
    uint32_t F:1; // [10] should be zero
    uint32_t Z:1; // [11] enables branch prediction.
    uint32_t I:1; // [12] enables lvl 1 instruction cache.
    uint32_t V:1; // [13] determines location of exception vectors.
    uint32_t RR:1; // [14] determines replacement strategy for cache.
    uint32_t L4:1; // [15] determines if T bit is set for PC load instructions.
    // b0 -> loads to PC set the T bit, reset val.
    // b1- > loads to PC don't set the T bit.
    uint32_t DT:1; // [16] deprecated
    uint32_t SBZ_1:1; // [17]
    uint32_t IT:1; // [18] deprecated
    uint32_t SBZ_2:2; // [20:19]
    uint32_t FI:1; // [21]
    uint32_t U:1; // [22] enables unaligned data access operations. b0 -> support disabled, reset val.
    uint32_t XP:1; // [23] enables extended page tables. b0 -> enabled, reset val.
    uint32_t VE:1; // [24] enables vic interface to determine interrupt vectors.
    uint32_t EE:1; // [25] determines how E bit on CPSR is set on an exception.
    uint32_t SBZ_3:2; // [27-26]
    uint32_t TR:1; // [28] controls tex remap functioanlity in the mmu.
    // b0-> disabled, reset val.
    uint32_t FA:1; // [29] controls access permission functionality in the mmu.
    // b0 -> disabled, reset val.
    uint32_t SBZ_4:2; // [31:30]
}c1_control_r;

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
    uint32_t SBZ_1:1; // [3]
    uint32_t PD0:1; // [4] b0-> enables pt walk for ttbr_0, reset val.
    uint32_t PD1:1; // [5] b0-> enables pt walk for ttbr_1, reset val.
    uint32_t SBZ_2:26; // [31:6]
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
all accesses are done to the correct context id. Use c7 cache operations reg for that.*/
typedef struct{
    uint32_t asid:8; // [7:0] basically procid but used by the hw in the tlb entries.
    uint32_t procid:24; // [31:8]
}c13_context_id_r;


/* PROGRAM STATUS REGISTERS */

#endif