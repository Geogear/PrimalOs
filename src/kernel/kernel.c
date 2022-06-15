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
#include <kernel/semaphore.h>
#include <kernel/mutex.h>
#include <common/string.h>
#include <common/system_user_api.h>

void test(void);
void keyboard_poller(void);
void synch1(void);
void synch2(void);
uint32_t k = 0, t = 0;

sem_t input_request;
sem_t input_data;
mutex_t input_lock;

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
    (void) r0;
    (void) r1;
    (void) atags;

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
    /*printf("INITIALIZING USB..");
    usb_init();
    printf("DONE\n");    */

    //printf("INITIALIZING SD HOST CONTROLLER...");
    //sd_card_init();
    //printf("DONE\n");
    puts("WELCOME TO PRIMAL OS!\n");

    char* thread_1 = "KEYBOARD_POLLER";
    char* thread_2 = "THREAD_2";
    char* user_thread = "USER_THREAD";

    sem_init(&input_request, 1);
    sem_init(&input_data, 1);
    mutex_init(&input_lock);

    //create_kernel_thread(keyboard_poller, thread_1, strlen(thread_1));
    
    //create_kernel_thread(test, thread_2, strlen(thread_2));
    for(uint32_t i = 0; i < SYS_USER_THREAD_COUNT; ++i)
        create_kernel_thread(sys_user_threads[i], user_thread, strlen(user_thread));
    create_kernel_thread(synch1, thread_2, strlen(thread_2));
    create_kernel_thread(synch2, thread_2, strlen(thread_2));

    while (1) {      
        printf("MAIN\t");
        udelay(1*SEC);
        //key_poll(1);
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

/*sem_t input_request;
sem_t input_data;
mutex_t input_lock;

void keyboard_poller(void){
    keyboard_enum();
    while(1){
        wait(input_request);
        // Need to poll in a loop until new line came.
        key_poll();
        signal(input_data);
    }    
}

#define LINE_BUF_SIZE 0
char* line_buf;

void get_line(void){
    mutex_lock(input_lock);
    signal(input_request);
    wait(input_data);
    transfer_input_data(line_buf, LINE_BUF_SIZE);
    mutex_unlock(input_lock);
}*/

void synch1(void){
    while(1){
        printf("SYNCH1 IR_W8\t");
        sem_wait(&input_request);
        printf("SYNCH1 ID_SIG\t");
        sem_signal(&input_data);
    }    
}

void synch2(void){
    mutex_lock(&input_lock);
    printf("SYNCH2 IR_SIG\t");
    sem_signal(&input_request);
    printf("SYNCH2 ID_W8\t");
    sem_wait(&input_data);
    mutex_unlock(&input_lock);
}