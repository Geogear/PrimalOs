#include <stddef.h>
#include <stdint.h>
#include "kernel.h"

#define GETBIT(S,N) (S >> N) & 0x1u
/* 
+ set operating mode to system in cpsr,
first read and print it, not sure of the reset val
+ in c1 register set all to b01.
*/

typedef struct{
    uint32_t hex0:4;
    uint32_t hex1:4;
    uint32_t hex2:4;
    uint32_t hex3:4;
    uint32_t hex4:4;
    uint32_t hex5:4;
    uint32_t hex6:4;
    uint32_t hex7:4;
}hexed;

char uint_to_hex_char(uint32_t h)
{
    if(h < 10)
        return (char)h + '0';
    return (char)h + '7';
}

static inline void mmio_write(uint32_t reg, uint32_t data)
{
    *(volatile uint32_t*)reg = data;
}

static inline uint32_t mmio_read(uint32_t reg)
{
    return *(volatile uint32_t*)reg;
}

// Loop <delay> times in a way that the compiler won't optimize away
static inline void delay(int32_t count)
{
    asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
            : "=r"(count): [count]"0"(count) : "cc");
}

enum
{
    // The GPIO registers base address.
    GPIO_BASE = 0x20200000, // 0x3F200000 for raspi2 & 3, 0x20200000 for raspi1 and 0

    // Controls actuation of pull up/down to ALL GPIO pins.
    GPPUD = (GPIO_BASE + 0x94),

    // Controls actuation of pull up/down for specific GPIO pin.
    GPPUDCLK0 = (GPIO_BASE + 0x98),

    // The base address for UART.
    UART0_BASE = 0x20201000, // 0x3F201000 for raspi2 & 3, 0x20201000 for raspi1 and 0

    // The offsets for reach register for the UART.
    UART0_DR     = (UART0_BASE + 0x00),
    UART0_RSRECR = (UART0_BASE + 0x04),
    UART0_FR     = (UART0_BASE + 0x18),
    UART0_ILPR   = (UART0_BASE + 0x20),
    UART0_IBRD   = (UART0_BASE + 0x24),
    UART0_FBRD   = (UART0_BASE + 0x28),
    UART0_LCRH   = (UART0_BASE + 0x2C),
    UART0_CR     = (UART0_BASE + 0x30),
    UART0_IFLS   = (UART0_BASE + 0x34),
    UART0_IMSC   = (UART0_BASE + 0x38),
    UART0_RIS    = (UART0_BASE + 0x3C),
    UART0_MIS    = (UART0_BASE + 0x40),
    UART0_ICR    = (UART0_BASE + 0x44),
    UART0_DMACR  = (UART0_BASE + 0x48),
    UART0_ITCR   = (UART0_BASE + 0x80),
    UART0_ITIP   = (UART0_BASE + 0x84),
    UART0_ITOP   = (UART0_BASE + 0x88),
    UART0_TDR    = (UART0_BASE + 0x8C),
};

void uart_init()
{
    // Disable UART0.
    mmio_write(UART0_CR, 0x00000000);
    // Setup the GPIO pin 14 && 15.

    // Disable pull up/down for all GPIO pins & delay for 150 cycles.
    mmio_write(GPPUD, 0x00000000);
    delay(150);

    // Disable pull up/down for pin 14,15 & delay for 150 cycles.
    mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));
    delay(150);

    // Write 0 to GPPUDCLK0 to make it take effect.
    mmio_write(GPPUDCLK0, 0x00000000);

    // Clear pending interrupts.
    mmio_write(UART0_ICR, 0x7FF);

    // Divider = 3000000 / (16 * 115200) = 1.627 = ~1.
    mmio_write(UART0_IBRD, 1);

    // Fractional part register = (.627 * 64) + 0.5 = 40.6 = ~40.
    mmio_write(UART0_FBRD, 40);

    // Enable FIFO & 8 bit data transmission (1 stop bit, no parity).
    mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

    // Mask all interrupts.
    mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
            (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));

    // Enable UART0, receive & transfer part of UART.
    mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

void uart_putc(unsigned char c)
{
    // Wait for UART to become ready to transmit.
    while ( mmio_read(UART0_FR) & (1 << 5) ) { }
    mmio_write(UART0_DR, c);
}

unsigned char uart_getc()
{
    // Wait for UART to have received something.
    while ( mmio_read(UART0_FR) & (1 << 4) ) { }
    return mmio_read(UART0_DR);
}

void uart_puts(const char* str)
{
    for (size_t i = 0; str[i] != '\0'; i ++)
        uart_putc((unsigned char)str[i]);
}

void log_uint(uint32_t num, char decision)
{
    int i = 0;
    char c = 'X';
    void* v = NULL;
    hexed h = {};
     switch(decision){
         case 'b':
            for(i = 31; i > -1; --i)
            {
                c = (GETBIT(num,i)) ? '1' : '0';
                uart_putc(c);
            }
        break;
        default:
            v = ((void*)(&num));
            h = *((hexed*)v);
            uart_putc(uint_to_hex_char(h.hex7));
            uart_putc(uint_to_hex_char(h.hex6));
            uart_putc(uint_to_hex_char(h.hex5));
            uart_putc(uint_to_hex_char(h.hex4));
            uart_putc(uint_to_hex_char(h.hex3));
            uart_putc(uint_to_hex_char(h.hex2));
            uart_putc(uint_to_hex_char(h.hex1));
            uart_putc(uint_to_hex_char(h.hex0));
     }
     uart_puts("\r\n");
}


void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
    (void) r0;
    (void) r1;
    (void) atags;

    uart_init();
    uart_puts("Welcome to Primal OS!\r\n");

    uint32_t dst = 0;

    __asm ("mrs %[dst], cpsr"
          : [dst] "=r" (dst));


    log_uint(dst, 'g');
    log_uint(dst, 'b');

    while (1) {
        uart_putc(uart_getc());
        uart_putc('\n');
    }
}