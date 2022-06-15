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
void test2(void);
void keyboard_enumarator(void);
void synch1(void);
void synch2(void);
uint32_t k = 0, t = 0;

sem_t input_request;
sem_t input_data;

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
    (void) r0;
    (void) r1;
    (void) atags;

    uart_init();
    mem_init((atag_t *)atags);
    if(!ON_EMU)  
        gpu_init();
    printf("INITIALIZING INTERRUPTS...");
    interrupts_init();
    printf("DONE\n");
    printf("INITIALIZING TIMER...");
    timer_init();
    printf("DONE\n");
    printf("INITIALIZING USB..");
    usb_init();
    printf("DONE\n");
    printf("INITIALIZING SCHEDULER...");
    process_init();
    printf("DONE\n");

    //printf("INITIALIZING SD HOST CONTROLLER...");
    //sd_card_init();
    //printf("DONE\n");
    printf("WELCOME TO PRIMAL OS!\n");
    //keyboard_enum();

    char* thread_1 = "KBOARD_ENUM";
    char* thread_2 = "THREAD_2";
    char* user_thread = "USER_THREAD";

    sem_init(&input_request, 1);
    sem_init(&input_data, 1);
    //create_kernel_thread(keyboard_enumarator, thread_1, strlen(thread_1));
    //create_kernel_thread(test, thread_2, strlen(thread_2));
    create_kernel_thread(test2, thread_2, strlen(thread_2));
    
    //create_kernel_thread(test, thread_2, strlen(thread_2));
    //for(uint32_t i = 0; i < SYS_USER_THREAD_COUNT; ++i)
        //create_kernel_thread(sys_user_threads[i], user_thread, strlen(user_thread));
    //create_kernel_thread(synch1, thread_2, strlen(thread_2));
    //create_kernel_thread(synch2, thread_2, strlen(thread_2));

    while (1) {      
        printf("MAIN\t");
        udelay(5*SEC);
        //key_poll(1);
    }
}

void keyboard_enumarator(void){
    printf("KEYENUM\t");
    keyboard_enum();
    //mutex_unlock(&input_lock);
}

void test(void) {
    uint8_t buf[32] = {};
    printf("ENTER TEST LINE: ");
    getline(buf, 32);
    printf("TEST LINE: %s", buf);
}

void test2(void) {
    uint8_t i = 0;
    while(i < 10){
        ++i;
        printf("TEST %d\t",i);
        udelay(3*SEC);
    }
}

void synch1(void){
    while(1){
        printf("SYNCH1 IR_W8\t");
        sem_wait(&input_request);
        printf("SYNCH1 ID_SIG\t");
        sem_signal(&input_data);
    }    
}

/*void synch2(void){
    mutex_lock(&input_lock);
    printf("SYNCH2 IR_SIG\t");
    sem_signal(&input_request);
    printf("SYNCH2 ID_W8\t");
    sem_wait(&input_data);
    mutex_unlock(&input_lock);
}*/