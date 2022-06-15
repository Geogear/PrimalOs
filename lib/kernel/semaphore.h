#include <kernel/process.h>
#include <kernel/mutex.h>
#ifndef SEM_H
#define SEM_H

/* If you change this you must also change the working details of the semaphore. */
#define TOTAL_SEMS 32

typedef struct{
    int val;
    uint32_t id;
}sem_t;

/* These are system semaphores. Only one thread should init a semaphore,
then share that semaphore with the cooperated threads to snychronize. 
Destroy call should only be made once.
*/

int sem_init(sem_t* sem, int val);
void sem_wait(sem_t* sem);
void sem_signal(sem_t* sem);
void sem_destroy(sem_t* sem);

#endif