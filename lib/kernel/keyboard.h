#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <kernel/peripheral.h>

struct key_press_report{
    uint8_t modifier_key;
    uint8_t reservered;
    unt8_t key_press_data[6];
};

void keyboard_irq_handler(void);

#endif