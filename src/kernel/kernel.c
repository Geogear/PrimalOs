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
void keyboard_poller(void);
uint32_t k = 0, t = 0;

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
    (void) r0;
    (void) r1;
    (void) atags;
    uint32_t i = 0;

    uart_init();
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

    //printf("INITIALIZING SD HOST CONTROLLER...");
    //sd_card_init();
    //printf("DONE\n");
    puts("WELCOME TO PRIMAL OS!\n");  

    char* thread_1 = "KEYBOARD_POLLER";
    char* thread_2 = "THREAD_2";

    //printf("THREAD CREATION...");
    create_kernel_thread(keyboard_poller, thread_1, strlen(thread_1));
    //printf("DONE\n");
    
    //create_kernel_thread(test, thread_2, strlen(thread_2));

    while (1) {
        if (i % 15 == 0){
            //sd_log_regs();
            //sd_read_cid_or_csd(10);
        }        
        //printf("MAIN %d\t", i++);
        //print_descrp();
        //dump_keyboard_info(1);
        udelay(1*SEC);
        key_poll(1);
        if(t == 6){
            //printf("THREAD CREATION...");
            //create_kernel_thread(test, thread_2, strlen(thread_2));
            //printf("DONE\n");
        }
    }
}

void keyboard_poller(void){
    keyboard_enum();
}

void test(void) {
    int i = 0, j = ++k;
    while (i < 10) {
        printf("THREAD%d-%d\t", j, i++);
        ++t;
        udelay(1000000);
    }
}