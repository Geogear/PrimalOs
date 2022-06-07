#include <kernel/peripheral.h>

inline void mmio_write(uint32_t* mem, uint32_t data){
	dmb();
	*(volatile uint32_t* )(mem) = data;
	dmb();
}

inline uint32_t mmio_read(uint32_t* mem){
	dmb();
	return *(volatile uint32_t*)(mem);
	dmb();
}

inline void mmio_write_direct(uint32_t address, uint32_t data){
    dmb();
    *(volatile uint32_t*)(address) = data;
    dmb();
}

uint32_t mmio_read_direct(uint32_t address){
    dmb();
    return *(volatile uint32_t*)(address);
    dmb();
}