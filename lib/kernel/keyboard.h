#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <kernel/peripheral.h>

void keyboard_irq_handler(void);

void keyboard_irq_clearer(void);

void keyboard_init(void);

void dump_keyboard_info(int o);

#endif