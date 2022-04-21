#include <kernel/uart.h>
#include <kernel/kernel.h>
#include <common/stdio.h>
#include <common/stdlib.h>

char getc(void) {
    return uart_getc();
}

void putc(char c) {
    uart_putc(c);
}

void puts(const char* str) {
    int i;
    for (i = 0; str[i] != '\0'; i ++)
        putc(str[i]);
}

void gets(char* buf, int buflen) {
    int i;
    char c;
    // Leave a spot for null char in buffer
    for (i = 0; (c = getc()) != '\r' && buflen > 1; i++, buflen--) {
        putc(c);
        buf[i] = c;
    }

    putc('\n');
    if (c == '\n') {
        buf[i] = '\0';
    }
    else
        buf[buflen-1] = '\0';
}

/* Used in log_uint, not exposed to outside. */
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