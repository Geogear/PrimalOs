#ifndef PERIPHERAL_H
#define PERIPHERAL_H

#include <stdint.h>

#define PERIPHERAL_BASE 0x20000000
#define PERIPHERAL_LENGTH 0x01000000

#define SYSTEM_TIMER_OFFSET 0x3000
#define USB_OFFSET 0x980000
#define INTERRUPTS_OFFSET 0xB000
#define MAILBOX_OFFSET 0xB880
#define UART0_OFFSET 0x201000
#define GPIO_OFFSET 0x200000
/* SD host controller  */
#define SDHCI_REGS_OFFSET 0x300000

enum board_power_feature {
    POWER_SD     = 0,
    POWER_UART_0 = 1,
    POWER_UART_1 = 2,
    POWER_USB    = 3,
};

extern void bcm2835_setpower(enum board_power_feature feature, uint32_t on);
extern void bcm2835_power_init(void);
extern void dmb(void);

#endif