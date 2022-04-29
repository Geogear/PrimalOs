#include <stddef.h>
#include <stdint.h>
#include <kernel/uart.h>
#include <kernel/kernel.h>

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
    asm volatile("__delay_%=: subs %[count], %[count], #1\n\t"
                "bne __delay_%=\n"
            : "=r"(count): [count]"0"(count) : "cc");
}

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

// Used in log_uint, not exposed to outside.
char uint_to_hex_char(uint32_t h)
{
    if(h < 10)
        return (char)h + '0';
    return (char)h + '7';
}

void log_uint(uint32_t num, char type)
{
    int i = 0;
    char c = 'X';
    void* v = NULL;
    hexed h = {};
     switch(type){
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