#include <stddef.h>
#include <stdint.h>
#include <kernel/kernel.h>
#include <kernel/atag.h>
#include <kernel/mem.h>
#include <kernel/uart.h>
#include <kernel/scp_regs.h>
#include <kernel/kerio.h>
#include <kernel/gpu.h>
#include <kernel/interrupts.h>
#include <kernel/timer.h>
#include <kernel/keyboard.h>
#include <kernel/usb.h>
#include <kernel/process.h>
#include <kernel/sd_host_regs.h>
#include <common/string.h>

void test(void);
uint32_t k = 0, t = 0;

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
    (void) r0;
    (void) r1;
    (void) atags;
    uint32_t i = 0;

    uart_init();

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
    //bcm2835_power_init();
    mem_init((atag_t *)atags);
    if(!ON_EMU)  
        gpu_init();
    printf("INITIALIZING INTERRUPTS...");
    interrupts_init();
    printf("DONE\n");
    printf("INITIALIZING TIMER...");
    timer_init();
    printf("DONE\n");
    printf("INITIALIZING SCHEDULER...");
    process_init();
    printf("DONE\n");
    printf("INITIALIZING USB..");
    usb_init();
    printf("DONE\n");    
    //printf("INITIALIZING KEYBOARD...");
    //keyboard_init();
    //printf("DONE\n");
    //dump_keyboard_info(0);

    //printf("INITIALIZING SD HOST CONTROLLER...");
    //sd_card_init();
    //printf("DONE\n");
    puts("WELCOME TO PRIMAL OS!\n");  

    char* thread_1 = "THREAD_1";
    char* thread_2 = "THREAD_2";

    //printf("THREAD CREATION...");
    //create_kernel_thread(test, thread_1, strlen(thread_1));
    //printf("DONE\n");
    
    //create_kernel_thread(test, thread_2, strlen(thread_2));
    int once = 1;
    while (1) {
        if (i % 15 == 0){
            //sd_log_regs();
            //sd_read_cid_or_csd(10);
        }        
        printf("MAIN %d\t", i++);
        //print_descrp();
        //dump_keyboard_info(1);
        udelay(2000000);

        if(once){
            once = 0;
            usb_poll(4);
        }
        if(t == 6){
            //printf("THREAD CREATION...");
            //create_kernel_thread(test, thread_2, strlen(thread_2));
            //printf("DONE\n");
        }
    }
}

void test(void) {
    int i = 0, j = ++k;
    while (i < 10) {
        printf("THREAD%d-%d\t", j, i++);
        ++t;
        udelay(1000000);
    }
}