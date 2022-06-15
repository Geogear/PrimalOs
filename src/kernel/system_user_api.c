#include <kernel/timer.h>
#include <common/system_user_api.h>

uint32_t get_time(void){
    return timer_get();
}

void busy_wait(uint32_t usecs){
    udelay(usecs);
}