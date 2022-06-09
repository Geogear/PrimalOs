#include <kernel/usb.h>
#include <kernel/usb_regs.h>
#include <kernel/interrupts.h>
#include <kernel/kerio.h>
#include <kernel/timer.h>
#include <kernel/usb_std_defs.h>

/** Round a number up to the next multiple of the word size.  */
#define WORD_ALIGN(n) (((n) + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1))

/** Determines whether a pointer is word-aligned or not.  */
#define IS_WORD_ALIGNED(ptr) ((uint32_t)(ptr) % sizeof(uint32_t) == 0)

/** TODO: remove this if appropriate */
#define START_SPLIT_INTR_TRANSFERS_ON_SOF 1

/**
 * Saved status of the host port.  This is modified when the host controller
 * issues an interrupt due to a host port status change.  The reason we need to
 * keep track of this status in a separate variable rather than using the
 * hardware register directly is that the changes in the hardware register need
 * to be cleared in order to clear the interrupt.
 */
static union dwc_host_port_ctrlstatus host_port_status;
static union dwc_host_channel_interrupts host_channel_int;

static volatile struct dwc_regs * const regs = (void*)USB_BASE;

static struct usb_device_descriptor dev_desc = {};
static struct usb_configuration_descriptor config_desc = {};
static struct usb_control_setup_data setup_data = {};
static uint8_t config_val = 0;
static uint8_t key_press_data[8] = {};

static enum device_status dev_stat = DEVICE_STATUS_COUNT;
static uint8_t dev_speed_low = 0;
static uint8_t control_phase = 0;
static uint8_t next_data_pid = DWC_USB_PID_DATA1;
static uint8_t transfer_total_size = 0;
static uint8_t transferred_size = 0;
static uint8_t active_address = 0;
static uint8_t transfer_active = 0;
static void* buf = NULL;
static void* dbuf = NULL;

static uint8_t prev_transfer = 255;
static uint8_t can_poll = 0;

/**
 * Read the Host Port Control and Status register with the intention of
 * modifying it.  Due to the inconsistent design of the bits in this register,
 * this requires zeroing the write-clear bits so they aren't unintentionally
 * cleared by writing back 1's to them.
 */
static union dwc_host_port_ctrlstatus
dwc_get_host_port_ctrlstatus(void)
{
    union dwc_host_port_ctrlstatus hw_status = regs->host_port_ctrlstatus;

    hw_status.enabled = 0;
    hw_status.connected_changed = 0;
    hw_status.enabled_changed = 0;
    hw_status.overcurrent_changed = 0;
    return hw_status;
}

/**
 * Powers on the DWC host port; i.e. the USB port that is logically attached to
 * the root hub.
 */
static void
dwc_power_on_host_port(void)
{
    union dwc_host_port_ctrlstatus hw_status;

    do{
        hw_status = dwc_get_host_port_ctrlstatus();
        hw_status.powered = 1;
        regs->host_port_ctrlstatus = hw_status;
        hw_status = dwc_get_host_port_ctrlstatus();
    }while(!hw_status.powered);
}

/**
 * Resets the DWC host port; i.e. the USB port that is logically attached to the
 * root hub.
 */
static void
dwc_reset_host_port(void)
{
    union dwc_host_port_ctrlstatus hw_status;

    /* Set the reset flag on the port, then clear it after a certain amount of
     * time.  */
    hw_status = dwc_get_host_port_ctrlstatus();
    hw_status.reset = 1;
    regs->host_port_ctrlstatus = hw_status;
    udelay(60 * MILI_SEC);  /* (sleep for 60 milliseconds).  */
    hw_status.reset = 0;
    regs->host_port_ctrlstatus = hw_status;
}

static inline uint32_t first_set_bit(uint32_t word){
    uint32_t i = 0;
    for(; i < 32; ++i){
        if((word >> i) & 1)
            break;
    }
    return i;
}

void dwc_soft_reset(void){
    /* Set soft reset flag, then wait until it's cleared.  */
    regs->core_reset = DWC_SOFT_RESET;
    while (regs->core_reset & DWC_SOFT_RESET)
    {
    }
}

static void dwc_setup_dma_mode(void){
    const uint32_t rx_words = 1024;  /* Size of Rx FIFO in 4-byte words */
    const uint32_t tx_words = 1024;  /* Size of Non-periodic Tx FIFO in 4-byte words */
    const uint32_t ptx_words = 1024; /* Size of Periodic Tx FIFO in 4-byte words */   

    regs->rx_fifo_size = rx_words;
    regs->nonperiodic_tx_fifo_size = (tx_words << 16) | rx_words;
    regs->host_periodic_tx_fifo_size = (ptx_words << 16) | (rx_words + tx_words);

    /* Actually enable DMA by setting the appropriate flag; also set an extra
     * flag available only in Broadcom's instantiation of the Synopsys USB block
     * that may or may not actually be needed.  */
    regs->ahb_configuration |= DWC_AHB_DMA_ENABLE | BCM_DWC_AHB_AXI_WAIT;          
}

static void control_transfer(void){
    union dwc_host_channel_transfer transfer_reg = regs->host_channels[0].transfer;
    dmb();
    union dwc_host_channel_characteristics char_reg = regs->host_channels[0].characteristics;
    void *data = NULL;

    char_reg.val = transfer_reg.val = 0;

    /* Default control endpoint.  The endpoint number, endpoint type, and
        * packets per frame are pre-determined, while the maximum packet size
        * can be found in the device descriptor.  */
    char_reg.endpoint_number = 0;
    char_reg.endpoint_type = USB_TRANSFER_TYPE_CONTROL;
    char_reg.max_packet_size = 8;
    char_reg.packets_per_frame = 1;
    char_reg.odd_frame = ((regs->host_frame_number & 0xffff) + 1) & 1;
    char_reg.device_address = active_address;
    regs->host_channels[0].characteristics = char_reg;

    switch (control_phase)
    {
        case 0:
            //setup phase
            char_reg.endpoint_direction = USB_DIRECTION_OUT;
            data = &setup_data;
            transfer_reg.size = sizeof(struct usb_control_setup_data);
            transfer_reg.packet_id = DWC_USB_PID_SETUP;
            control_phase = (setup_data.wLength == 0) ? 2 : 1;
            next_data_pid = DWC_USB_PID_DATA1;
            break;
        case 1:
            //data phase
            char_reg.endpoint_direction = setup_data.bmRequestType >> 7;
            /* We need to carefully take into account that we might be
                 * re-starting a partially complete transfer.  */
            data = buf + transferred_size;
            transferred_size += char_reg.max_packet_size;
            if(transferred_size >= transfer_total_size){
                control_phase = 2;
            }
            transfer_reg.packet_id = next_data_pid;
            next_data_pid = (next_data_pid == DWC_USB_PID_DATA1) ? DWC_USB_PID_DATA0 : DWC_USB_PID_DATA1;
            break;        
        default:
            // status phase
            /* The direction of the STATUS transaction is opposite the
                * direction of the DATA transactions, or from device to host if
                * there were no DATA transactions.  */
            if((setup_data.bmRequestType >> 7) == USB_DIRECTION_OUT || setup_data.wLength == 0){
                char_reg.endpoint_direction = USB_DIRECTION_IN;
            }else{
                char_reg.endpoint_direction = USB_DIRECTION_OUT;
            }
            /* The STATUS transaction has no data buffer, yet must use a
                * DATA1 packet ID.  */
            data = NULL;
            transfer_reg.size = 0;
            transfer_reg.packet_id = DWC_USB_PID_DATA1;
            control_phase = 0;
            break;
    }
    transfer_reg.packet_count = 1;
    char_reg.channel_enable = 1;

    /* Actually program the registers of the appropriate channel.  */
    regs->host_channels[0].transfer = transfer_reg;
    regs->host_channels[0].dma_address = data;
    regs->host_channels[0].characteristics = char_reg;
}

static void usb_interrupt_handler(void){
    DISABLE_INTERRUPTS();
    printf("USB_HANDLER\t");
    //printf("CORE INTR:%x\tHOST CHNLS: %x\tPORT STAT:%x\tCHAN 0 INT:%x\t",
    //regs->core_interrupts.val, regs->host_channels_interrupt,
    //regs->host_port_ctrlstatus.val, regs->host_channels[0].interrupt_mask.val);

    uint8_t prev_phase = 0;
    if(transfer_active){
        switch (dev_stat)
        {
            case POWERED:
                udelay(120*MILI_SEC);
                dwc_reset_host_port();
                printf("POW-PORT:%x\t",regs->host_port_ctrlstatus);
                printf("SPED:%d\t",host_port_status.speed);
                dev_speed_low = (host_port_status.speed == USB_SPEED_LOW) ? 1 : 0;
                dev_stat = RESET;
                break;
            case RESET:
                printf("RESET\t");
                // Set address setup request
                setup_data.bmRequestType = USB_BMREQUESTTYPE_DIR_OUT | USB_REQUEST_TYPE_STANDARD
                | USB_REQUEST_RECIPIENT_DEVICE; // A length way to say zero, lel.
                setup_data.bRequest = USB_DEVICE_REQUEST_SET_ADDRESS;
                setup_data.wValue = KEYBOARD_DEVICE_ADDRESS;
                setup_data.wIndex = setup_data.wLength = 0;
                buf = &setup_data; dbuf = NULL; transfer_total_size = sizeof(setup_data); transferred_size = 0;

                // Enable channel 0
                regs->host_channels_interrupt_mask = 1;
                regs->host_channels[0].interrupt_mask.val = 0x3ff;

                prev_phase = control_phase;
                control_transfer();
                if(prev_phase == 2 && control_phase == 0){
                    active_address = KEYBOARD_DEVICE_ADDRESS;
                    dev_stat = ADDRESSED;
                    transfer_active = 0;
                    udelay(100*MILI_SEC); //Give the device recovery time.
                }
                break;
            case ADDRESSED:
                printf("ADDRESSED\t");
                // Boot protocol control request setup stage.
                /*setup_data.bmRequestType = 0x21;
                setup_data.bRequest = 0x0b;
                setup_data.wValue = 0; // 0 for boot protocol, 1 for report protocol
                setup_data.wIndex = setup_data.wLength = 0;
                buf = &setup_data; transfer_total_size = sizeof(setup_data); transferred_size = 0;*/

                // Enable channel interrupts
                regs->host_channels[0].interrupt_mask.val = 0x3ff;

                prev_phase = control_phase;
                control_transfer();
                if(prev_phase == 2 && control_phase == 0){
                    transfer_active = 0;
                }
                if(prev_phase == 0 && control_phase == 1){
                    buf = dbuf;
                    transfer_total_size = setup_data.wLength;
                    transferred_size = 0;
                    can_poll = 1;
                }
                break;
            default:
                printf("DEFAULT\t");
        }
    }else if(transfer_active == 0 && prev_transfer != 255){
        switch (prev_transfer)
        {
            case 0: // get_status
                printf("PREV_TRANSFER: %d", prev_transfer);
                break;
            case 1: // get config
                printf("PREV_TRANSFER: %d", prev_transfer);
                break;
            case 2: // set config
                printf("PREV_TRANSFER: %d", prev_transfer);
                break;
            case 3: // get report, hdi keyboard specific
                printf("PREV_TRANSFER: %d", prev_transfer);
                break;
            case 4: // get config desc
                printf("CONFIG DESC LENGLTH: %x, DESCRPTR_TYPE: %x, TOTAL_LEN: %x, NUM_INTRFCS: %x, CONFIG_VAL: %x,"
                " ICONFIG: %x, ATTRBTS: %x, MAX_POWER: %x\t",
                config_desc.bLength, config_desc.bDescriptorType, config_desc.wTotalLength, config_desc.bNumInterfaces,
                config_desc.bConfigurationValue, config_desc.iConfiguration, config_desc.bmAttributes, config_desc.bMaxPower);
                config_val = config_desc.bConfigurationValue;
                break;
            default:
                printf("ERR: UNEXPECTED VAL %d PREV_TRANSFER.\t", prev_transfer);
                break;
        }
        prev_transfer = 255;
    }

    ENABLE_INTERRUPTS();
}

static void usb_interrupt_clearer(void){
    printf("USB CLEARER\tPORT STAT:%x\tCORE INTR:%x\t",regs->host_port_ctrlstatus,
    regs->core_interrupts.val);
    if (regs->core_interrupts.port_intr){
        printf("PORT INTR IN\t");
        /* Clear the interrupt(s), which are "write-clear", by writing the Host
         * Port Control and Status register back to itself.  But as a special
         * case, 'enabled' must be written as 0; otherwise the port will
         * apparently disable itself.  */
        host_port_status = regs->host_port_ctrlstatus;
        host_port_status.enabled = 0;
        regs->host_port_ctrlstatus = host_port_status;
    }

    // Handle connect/disconnect
    if(host_port_status.connected && dev_stat == DEVICE_STATUS_COUNT){
        dev_stat = POWERED;
    }else if(!host_port_status.connected){
        dev_stat = DEVICE_STATUS_COUNT;
    }

    host_channel_int = regs->host_channels[0].interrupts;
    printf("CHNL:%x\t", host_channel_int.val);
    if(regs->core_interrupts.host_channel_intr){
        /* Clear pending interrupts.  */
        regs->host_channels[0].interrupt_mask.val = 0;
        regs->host_channels[0].interrupts.val = 0xffffffff;
    }
}

static void dwc_setup_interrupts(void){
    union dwc_core_interrupts core_interrupt_mask;
    /* Clear all pending core interrupts.  */
    regs->core_interrupt_mask.val = 0;
    regs->core_interrupts.val = 0xffffffff;

    /* Enable core host channel and port interrupts.  */
    core_interrupt_mask.val = 0;
    core_interrupt_mask.host_channel_intr = 1;
    core_interrupt_mask.port_intr = 1;
    regs->core_interrupt_mask = core_interrupt_mask;

    /* Enable the interrupt line that goes to the USB controller and register
     * the interrupt handler.  */
    register_irq_handler(USB_CONTROLER, usb_interrupt_handler, usb_interrupt_clearer);

    /* Enable interrupts for entire USB host controller.  (Yes that's what we
     * just did, but this one is controlled by the host controller itself.)  */
    regs->ahb_configuration |= DWC_AHB_INTERRUPT_ENABLE;
}

void usb_init(void){
    DISABLE_INTERRUPTS();
    if(!ON_EMU)
        bcm2835_setpower(POWER_USB, 1);
    dwc_soft_reset();
    dwc_setup_dma_mode();
    dwc_power_on_host_port();
    dwc_setup_interrupts();
    ENABLE_INTERRUPTS();
}

void usb_poll(uint8_t request_type){
    union dwc_host_channel_transfer transfer_reg;
    union dwc_host_channel_characteristics char_reg;
    union dwc_host_channel_split_control split_control;
    void *data;
    uint32_t next_frame;
    if (can_poll){
        prev_transfer = request_type;
        can_poll = 0;
        printf("INPOL\t");

        switch (request_type)
        {
            case 0: // get_status
                break;
            case 1: // get config
                setup_data.bmRequestType = 1 >> 7;
                setup_data.bRequest = USB_DEVICE_REQUEST_GET_CONFIGURATION;
                setup_data.wValue = setup_data.wIndex = 0;
                setup_data.wLength = 1;
                dbuf = &config_val;
                break;
            case 2: // set config
                setup_data.bmRequestType = 0;
                setup_data.bRequest = USB_DEVICE_REQUEST_SET_CONFIGURATION;
                setup_data.wValue = config_val;
                setup_data.wIndex = setup_data.wLength = 0;
                dbuf = NULL;
                break;
            case 3: // get report, hdi keyboard specific
                setup_data.bmRequestType = 0xa1;
                setup_data.bRequest = 0x01;
                setup_data.wValue = 0x0100;
                setup_data.wIndex = 0x0;
                setup_data.wLength = sizeof(key_press_data);
                dbuf = &key_press_data[0];
                break;
            case 4: // get config desc
                setup_data.bmRequestType = 1 >> 7;
                setup_data.bRequest = USB_DEVICE_REQUEST_GET_DESCRIPTOR;
                setup_data.wValue = USB_DESCRIPTOR_TYPE_CONFIGURATION >> 7;
                setup_data.wIndex = 0;
                setup_data.wLength = sizeof(config_desc);
                dbuf = &config_desc;
                break;        
            default:
                printf("ERR: UNEXPECTED VAL %d for REQUEST_TYPE.\t", request_type);
                break;
        }
        buf = &setup_data; transfer_total_size = sizeof(setup_data); transferred_size = 0; 

        // Enable channel interrupts
        regs->host_channels[0].interrupt_mask.val = 0x3ff;
        control_transfer();
    }
}