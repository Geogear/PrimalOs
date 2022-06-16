#include <kernel/timer.h>
#include <common/system_user_api.h>

/* Define your functions. */
void thread1(void);
void thread2(void);
void mastermind(void);
void metric(void);

/* Implement your functions. */
void thread1(void) {
    int i = 0;
    while (i < 10) {
        printf("THREAD 1\t");
        ++i;
        udelay(1000000);
    }
}

void thread2(void) {
    int i = 0;
    while (i < 20) {
        printf("THREAD 2\t");
        ++i;
        udelay(1000000);
    }
}

void master_mind(void){
    char* thread_text = "MASTERMIND >>";
    char number_buf[11] = {};
    uint32_t entered_number = 0;
    uint32_t decided_number = 0;

    printf("\n%sWELCOME TO MASTERMIND NUMBER GUESS GAME, ENTER MAX RANGE: ", thread_text);
    getline((uint8_t*)number_buf, 11);
    entered_number = atoi(number_buf);
    srand(get_time());
    decided_number = divmod(rand(), entered_number).mod;
    printf("\n%sDECIDED NUM: %d", thread_text, decided_number);

    do{
        for(uint32_t i = 0; i<11; ++i)
            number_buf[i] = '\0';
        printf("\n%sGUESS A NUMBER:",thread_text);
        getline((uint8_t*)number_buf, 11);
        entered_number = atoi(number_buf);

        if(entered_number > decided_number)
            printf("\n%sGO LOWER!",thread_text);
        else if (entered_number < decided_number)
            printf("\n%sGO HIGHER!",thread_text);

    }while(entered_number != decided_number);

    printf("\n%sYOU HAVE GUESSED CORRECTLY!",thread_text);
    printf("\n%sEXITING.",thread_text);
}

void metric(void){
    char* thread_text = "METRIC >>";
    uint32_t total_time = get_time();
    uint32_t percentage = 100 / (total_time / system_time);
    printf("\n%sSYSTEM TIME: %d && TOTAL TIME: %d -> %d PERCENT SYSTEM TIME.\n", 
    thread_text, system_time, total_time, percentage);
}

/* Addresses of the functions. */
extern thread_function_f sys_user_threads[SYS_USER_THREAD_COUNT] = 
{
    thread1,
    thread2,
    master_mind,
    metric
};

/* Names of the functions are used to run them from the shell. */
extern char sut_names[SYS_USER_THREAD_COUNT][20] = {
    "THREAD1",
    "THREAD2",
    "MASTERMIND",
    "METRIC"
};

/* Set as 1, to run at the start. */
extern uint8_t sut_run_at_start[SYS_USER_THREAD_COUNT] = {
    1,
    1,
    0,
    0
};