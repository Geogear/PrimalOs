/**
 * @file bcm2835_power.c
 *
 * This driver provides the ability to power on and power off hardware, such as
 * the USB Controller, on the BCM2835 SoC used on the Raspberry Pi.  This makes
 * use of the BCM2835's mailbox mechanism.
 */
#include <kernel/peripheral.h>
#include <stdint.h>

static volatile uint32_t *const mailbox_regs = (volatile uint32_t*)(PERIPHERAL_BASE + MAILBOX_OFFSET);

/* BCM2835 mailbox register indices  */
#define MAILBOX_READ               0
#define MAILBOX_STATUS             6
#define MAILBOX_WRITE              8

/* BCM2835 mailbox status flags  */
#define MAILBOX_FULL               0x80000000
#define MAILBOX_EMPTY              0x40000000

/* BCM2835 mailbox channels  */
#define MAILBOX_CHANNEL_POWER_MGMT 0

/* The BCM2835 mailboxes are used for passing 28-bit messages.  The low 4 bits
 * of the 32-bit value are used to specify the channel over which the message is
 * being transferred  */
#define MAILBOX_CHANNEL_MASK       0xf

/* Write to the specified channel of the mailbox.  */
static void
bcm2835_mailbox_write(uint32_t channel, uint32_t value)
{
    while (mailbox_regs[MAILBOX_STATUS] & MAILBOX_FULL)
    {
    }
    mailbox_regs[MAILBOX_WRITE] = (value & ~MAILBOX_CHANNEL_MASK) | channel;
}

/* Read from the specified channel of the mailbox.  */
static uint32_t
bcm2835_mailbox_read(uint32_t channel)
{
    uint32_t value;

    while (mailbox_regs[MAILBOX_STATUS] & MAILBOX_EMPTY)
    {
    }
    do
    {
        value = mailbox_regs[MAILBOX_READ];
    } while ((value & MAILBOX_CHANNEL_MASK) != channel);
    return (value & ~MAILBOX_CHANNEL_MASK);
}

/* Retrieve the bitmask of power on/off state.  */
static uint32_t
bcm2835_get_power_mask(void)
{
    return (bcm2835_mailbox_read(MAILBOX_CHANNEL_POWER_MGMT) >> 4);
}

/* Set the bitmask of power on/off state.  */
static void
bcm2835_set_power_mask(uint32_t mask)
{
    bcm2835_mailbox_write(MAILBOX_CHANNEL_POWER_MGMT, mask << 4);
}

/* Bitmask that gives the current on/off state of the BCM2835 hardware.
 * This is a cached value.  */
static uint32_t bcm2835_power_mask;

/**
 * Powers on or powers off BCM2835 hardware.
 *
 * @param feature
 *      Device or hardware to power on or off.
 * @param on
 *      ::TRUE to power on; ::FALSE to power off.
 *
 * @return
 *      ::OK if successful; ::SYSERR otherwise.
 */
void bcm2835_setpower(enum board_power_feature feature, uint32_t on)
{
    uint32_t bit;
    uint32_t newmask;
    uint32_t is_on;

    bit = 1 << feature;
    is_on = (bcm2835_power_mask & bit) != 0;
    if (on != is_on)
    {
        newmask = bcm2835_power_mask ^ bit;
        //TODO this might nıt be successfull
        bcm2835_set_power_mask(newmask);
        bcm2835_power_mask = bcm2835_get_power_mask();
    }
}

/**
 * Resets BCM2835 power to default state (all devices powered off).
 */
void bcm2835_power_init(void)
{
    bcm2835_set_power_mask(0);
    bcm2835_power_mask = 0;
}