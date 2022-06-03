#include <kernel/peripheral.h>

inline void mmio_write(uint32_t* mem, uint32_t data)
{
	dmb();
	*(volatile)mem = data;
	dmb();
}

inline uint32_t mmio_read(uint32_t* mem)
{
	dmb();
	return *(volatile)mem;
	dmb();
}