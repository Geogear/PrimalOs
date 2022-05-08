#include <kernel/usb.h>

void usb_init(void){
    bcm2835_setpower(POWER_USB, 1);
}