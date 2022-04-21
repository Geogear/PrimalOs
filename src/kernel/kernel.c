#include <stddef.h>
#include <stdint.h>
#include <kernel/kernel.h>
#include <kernel/uart.h>
#include <kernel/registers.h>
#include <common/stdio.h>

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
    (void) r0;
    (void) r1;
    (void) atags;
    void* v = NULL;

    uart_init();
    uart_puts("Welcome to Primal OS!\r\n");

    uint32_t dst = 0, tmp = 0;
    /* 
        + in c1 register set all to b01.
    */

    /* Enable access for domain control access register. */
    v = ((void*)(&dst));
    c3_domain_access_control_r* dac_reg = (c3_domain_access_control_r*)v;
    dst = 0xffffffff;

    /* Write the changed value. */
    __asm volatile( "mcr p15, 0, %[dst], c1, c0, 2"
          :
          : [dst] "r" (dst));

    /* Write the changed value. */
    __asm volatile( "mcr p15, 0, %[dst], c10, c0, 0"
          :
          : [dst] "r" (dst));

    /* Read back to confirm the value is changed. */
    __asm volatile("mrc p15, 0, %[tmp], c10, c0, 0"
          : [tmp] "=r" (tmp));

    log_uint(tmp, 'h');
    log_uint(tmp, 'b');              

    /* Read back to confirm the value is changed. */
    __asm volatile("mrc p15, 0, %[tmp], c1, c0, 2"
          : [tmp] "=r" (tmp));

    log_uint(tmp, 'h');
    log_uint(tmp, 'b');                  
               
    while (1) {
        uart_putc(uart_getc());
        uart_putc('\n');
    }
}