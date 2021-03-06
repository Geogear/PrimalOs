#include <kernel/interrupts.h>
#include <kernel/kerio.h>
#include <kernel/timer.h>
#include <kernel/process.h>
#include <common/stdlib.h>
#include <kernel/uart.h>

static interrupt_registers_t * interrupt_regs;

static interrupt_handler_f handlers[NUM_IRQS];
static interrupt_clearer_f clearers[NUM_IRQS];

extern void move_exception_vector(void);
extern uint32_t exception_vector;

void interrupts_init(void) {
    interrupt_regs = (interrupt_registers_t *)INTERRUPTS_PENDING;
	bzero(handlers, sizeof(interrupt_handler_f) * NUM_IRQS);
	bzero(clearers, sizeof(interrupt_clearer_f) * NUM_IRQS);
	interrupt_regs->irq_basic_disable = 0xffffffff; // disable all interrupts
	interrupt_regs->irq_gpu_disable1 = 0xffffffff;
	interrupt_regs->irq_gpu_disable2 = 0xffffffff;
    move_exception_vector();
    ENABLE_INTERRUPTS();
}

/**
 * this function is going to be called by the processor.  Needs to check pending interrupts and execute handlers if one is registered
 */
void irq_handler(void) {
    DISABLE_INTERRUPTS();
    uint32_t last_time = timer_get();
    int j;
    for (j = 0; j < NUM_IRQS; j++) {
        if (IRQ_IS_PENDING(interrupt_regs, j) && j != SYSTEM_TIMER_1){
            //*printf("PENDING IRQ %d\t",j);
            udelay(500);
        }
    } 
	for (j = 0; j < NUM_IRQS; j++) {
        // If the interrupt is pending and there is a handler, run the handler
        if (IRQ_IS_PENDING(interrupt_regs, j)  && (handlers[j] != 0)) {
			clearers[j]();
			//ENABLE_INTERRUPTS();
			handlers[j]();
			//DISABLE_INTERRUPTS();
			return;
        }
    }
    system_time += timer_get() - last_time;
    ENABLE_INTERRUPTS();
}

void __attribute__ ((interrupt ("ABORT"))) reset_handler(void) {
    printf("RESET HANDLER\n");
    while(1);
}
void __attribute__ ((interrupt ("ABORT"))) prefetch_abort_handler(void) {
    printf("PREFETCH ABORT HANDLER\n");
    while(1);
}
void __attribute__ ((interrupt ("ABORT"))) data_abort_handler(void) {
    printf("DATA ABORT HANDLER\n");
    while(1);
}
void __attribute__ ((interrupt ("UNDEF"))) undefined_instruction_handler(void) {
    printf("UNDEFINED INSTRUCTION HANDLER\n");
    while(1);
}
void __attribute__ ((interrupt ("SWI"))) software_interrupt_handler(void) {
    printf("SWI HANDLER\n");
    while(1);
}
void __attribute__ ((interrupt ("FIQ"))) fast_irq_handler(void) {
    printf("FIQ HANDLER\n");
    while(1);
}




void register_irq_handler(irq_number_t irq_num, interrupt_handler_f handler, interrupt_clearer_f clearer) {
    uint32_t irq_pos;
    //*printf("IRQ REGISTER %d\n",irq_num);
    if (IRQ_IS_BASIC(irq_num)) {
        //*printf("IRQ BASIC\n");
        irq_pos = irq_num - 64;
        handlers[irq_num] = handler;
		clearers[irq_num] = clearer;
        interrupt_regs->irq_basic_enable |= (1 << irq_pos);
    }
    else if (IRQ_IS_GPU2(irq_num)) {
        irq_pos = irq_num - 32;
        handlers[irq_num] = handler;
		clearers[irq_num] = clearer;
        interrupt_regs->irq_gpu_enable2 |= (1 << irq_pos);
    }
    else if (IRQ_IS_GPU1(irq_num)) {
        //*printf("IRQ GPU1\n");
        irq_pos = irq_num;
        handlers[irq_num] = handler;
		clearers[irq_num] = clearer;
        interrupt_regs->irq_gpu_enable1 |= (1 << irq_pos);
    }
    else {
        printf("ERROR: CANNOT REGISTER IRQ HANDLER: INVALID IRQ NUMBER: %d\n", irq_num);
    }
}
void unregister_irq_handler(irq_number_t irq_num) {
    uint32_t irq_pos;
    if (IRQ_IS_BASIC(irq_num)) {
        irq_pos = irq_num - 64;
        handlers[irq_num] = 0;
        clearers[irq_num] = 0;
        // Setting the disable bit clears the enabled bit
        interrupt_regs->irq_basic_disable |= (1 << irq_pos);
    }
    else if (IRQ_IS_GPU2(irq_num)) {
        irq_pos = irq_num - 32;
        handlers[irq_num] = 0;
        clearers[irq_num] = 0;
        interrupt_regs->irq_gpu_disable2 |= (1 << irq_pos);
    }
    else if (IRQ_IS_GPU1(irq_num)) {
        irq_pos = irq_num;
        handlers[irq_num] = 0;
        clearers[irq_num] = 0;
        interrupt_regs->irq_gpu_disable1 |= (1 << irq_pos);
    }
    else {
        printf("ERROR: CANNOT UNREGISTER IRQ HANDLER: INVALID IRQ NUMBER: %d\n", irq_num);
    }
}