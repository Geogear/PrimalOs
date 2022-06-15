#include <common/system_user_api.h>

/* Define your functions. */
void thread1(void);
void thread2(void);

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
    while (i < 10) {
        printf("THREAD 2\t");
        ++i;
        udelay(1000000);
    }
}

/* Store them in the array. Main thread will create these
threads after system initialization. */
extern thread_function_f sys_user_threads[SYS_USER_THREAD_COUNT] = 
{
    thread1,
    thread2
};