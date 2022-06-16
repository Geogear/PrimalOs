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

void synch1(void);
void synch2(void);
void empty(void);

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
    printerr("INITIALIZING INTERRUPTS...");
    interrupts_init();
    printerr("DONE\n");
    printerr("INITIALIZING TIMER...");
    timer_init();
    printerr("DONE\n");
    printerr("INITIALIZING USB..");
    usb_init();
    printerr("DONE\n");
    printerr("KEYBOARD ENUMARATION");
    keyboard_enum();
    printerr("INITIALIZING SCHEDULER...");
    process_init();
    printerr("DONE\n");

    //printf("INITIALIZING SD HOST CONTROLLER...");
    //sd_card_init();
    //printf("DONE\n");

    printerr("WELCOME TO PRIMAL OS!\n");

    sem_init(&input_request, 1);
    sem_init(&input_data, 1);
    
    // Runs the threads at the start that are set in the 
    // sut_run_at_start, array.
    for(uint32_t i = 0; i < SYS_USER_THREAD_COUNT; ++i){
        if(sut_run_at_start[i])
            create_kernel_thread(sys_user_threads[i], sut_names[i], strlen(sut_names[i]));
    }
    create_kernel_thread(synch1, "SYNCH1", strlen("SYNCH1"));
    create_kernel_thread(synch2, "SYNCH2", strlen("SYNCH2"));
    // IMPORTANT: We always need a thread running for scheduling.
    create_kernel_thread(empty, "EMPTY", strlen("EMPTY"));

    char* shell_text = "PRIME-SHELL >>";
    uint8_t shell_buf[128];
    uint32_t match = 0;
    system_time = get_time();
    while (1) {
        for(uint32_t i = 0; i<128; ++i)
            shell_buf[i] = 0;
        printf("\n%s", shell_text);
        getline(shell_buf, 128);

        if(shell_buf[0] == '\0')
            yield();
        else{
            match = 0;
            for(uint32_t i = 0; i < SYS_USER_THREAD_COUNT; ++i){
                if(strequal(sut_names[i], (char*)shell_buf)){
                    match = i;
                    break;
                }
            }
            if(!match)
                printf("\n%sGIVEN NAME IS INVALID!",shell_text);
            else
                create_kernel_thread(sys_user_threads[match], sut_names[match], strlen(sut_names[match]));
            yield();
        }
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

void synch2(void){
    printf("SYNCH2 IR_SIG\t");
    sem_signal(&input_request);
    printf("SYNCH2 ID_W8\t");
    sem_wait(&input_data);
}

void empty(void){
    while(1){
        udelay(1*SEC);
    }
}