#ifndef SYS_USER_API_H
#define SYS_USER_API_H

/* You should not be concerned with this, if you're just writing your own threads.
If you'd like to add your own schedule algorithms, you should dwell into process.h/c. */
#include <kernel/process.h>

/* You should be concerned with these when you write you own threads. */
#include <kernel/kerio.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <common/stdlib.h>
#include <common/string.h>

/* Additional to the functions here, system user should be concerned with the included headers
as well. System user will find that most of them replicas of standard library functions, so they
can expand these implementations, if they so desire. Some are synchronization functions.

Inside stdlib.h, we have uidiv/uidivmod functions. These are software emulated modulus and division
operations, since the hardware can't do them. When you do a division or modulus operation. They
are linked to these functions. */

/* Returns the current time in microseconds. */
uint32_t get_time(void);

/* Busy waits for given microseconds. */
void busy_wait(uint32_t usecs);

/* Main thread will create your functions, at the start from this array.
So you should set the count accordingly and fill the array.
This is just declaration of the array, for the definition find the system_user_threads.c 
You can also implement your threads there. */
#define SYS_USER_THREAD_COUNT 4
extern thread_function_f sys_user_threads[SYS_USER_THREAD_COUNT];

extern char sut_names[SYS_USER_THREAD_COUNT][20];

extern uint8_t sut_run_at_start[SYS_USER_THREAD_COUNT];

#endif