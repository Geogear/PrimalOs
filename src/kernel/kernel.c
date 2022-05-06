#include <stddef.h>
#include <stdint.h>
#include <kernel/kernel.h>
#include <kernel/atag.h>
#include <kernel/mem.h>
#include <kernel/uart.h>
#include <kernel/registers.h>
#include <kernel/kerio.h>
#include <kernel/gpu.h>
#include <kernel/interrupts.h>
#include <kernel/timer.h>

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
    (void) r0;
    (void) r1;
    (void) atags;
    int i = 0;

    uart_init();
    uart_puts("Welcome to Primal OS!\r\n");

    /*uint32_t dst = 0, tmp = 0;
    void* v = NULL;
      /
        + in c1 register set all to b01.
      /
   
    / Enable access for domain control access register. /
    v = ((void*)(&dst));
    c3_domain_access_control_r* dac_reg = (c3_domain_access_control_r*)v;
    dst = 0xffffffff;

    / Write the changed value. /
    __asm volatile( "mcr p15, 0, %[dst], c1, c0, 2"
          :
          : [dst] "r" (dst));

    / Write the changed value. /
    __asm volatile( "mcr p15, 0, %[dst], c10, c0, 0"
          :
          : [dst] "r" (dst));

    / Read back to confirm the value is changed. /
    __asm volatile("mrc p15, 0, %[tmp], c10, c0, 0"
          : [tmp] "=r" (tmp));

    log_uint(tmp, 'h');
    log_uint(tmp, 'b');              

    / Read back to confirm the value is changed. /
    __asm volatile("mrc p15, 0, %[tmp], c1, c0, 2"
          : [tmp] "=r" (tmp));

    log_uint(tmp, 'h');
    log_uint(tmp, 'b');     */

    //puts("Welcome to Primal OS!!\n");
    mem_init((atag_t *)atags);
    uart_puts("gpu init");
    //gpu_init();
    //printf("GPU INITIALIZED\n");
    //printf("INITIALIZING INTERRUPTS...");
    uart_puts("interrupts init");
    interrupts_init();
    //printf("DONE\n");
    //printf("INITIALIZING TIMER...");
    uart_puts("timer init");
    timer_init();
    //printf("DONE\n");    

    uart_puts("timer set");  
    timer_set(3000000);
    uart_puts("loop");           
    while (1) {
        uartf("main %d\n", i++);//printf("main %d\n", i++);
        udelay(1000000);
    }
}