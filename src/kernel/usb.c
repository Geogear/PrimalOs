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

static struct usb_device_descriptor descriptor = {};
static struct usb_control_setup_data setup_data;
static uint8_t key_press_data[8] = {};

static enum device_status dev_stat = POWERED;

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

    hw_status = dwc_get_host_port_ctrlstatus();
    hw_status.powered = 1;
    regs->host_port_ctrlstatus = hw_status;
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

static void usb_interrupt_handler(void){
    DISABLE_INTERRUPTS();
    printf("USB_HANDLER\t");
    printf("CORE INTR:%x\tHOST CHNLS: %x\tPORT STAT:%x\tCHAN 0 INT:%x\t",
    regs->core_interrupts.val, regs->host_channels_interrupt,
    regs->host_port_ctrlstatus.val, regs->host_channels[0].interrupt_mask.val);

    union dwc_core_interrupts interrupts = regs->core_interrupts;
    if(interrupts.sof_intr){
        //printf("SOF INTR HOST FRAME NUM:%x\t", regs->host_frame_number);
        if ((regs->host_frame_number & 0x7) != 6){
            printf("SOF INTR IN\t");
            union dwc_core_interrupts tmp;

            // Disable SOF interrupt if no longer needed
            //if (sofwait == 0)
            //{
                tmp = regs->core_interrupt_mask;
                tmp.sof_intr = 0;
                regs->core_interrupt_mask = tmp;
            //}

            /* Clear SOF interrupt */
            tmp.val = 0;
            tmp.sof_intr = 1;
            regs->core_interrupts = tmp; 
        }
    }
    if (interrupts.host_channel_intr){
        printf("HOST CH INTR\t");
        // One or more channels has an interrupt pending.
        uint32_t chintr;
        uint8_t chan;

        chintr = regs->host_channels_interrupt;
        do
        {
            chan = first_set_bit(chintr);
            if(chan){
                printf("CHAN %d\tTRNSFR VAL: %x\tCHRCTERSTCS:%x\tSC: %x\tINTRPS: %x\t",
                chan, regs->host_channels[chan].transfer.val,
                regs->host_channels[chan].characteristics.val,
                regs->host_channels[chan].split_control.val,
                regs->host_channels[chan].interrupts.val);
            }
            chintr ^= (1 << chan);
        } while (chintr != 0);        
    }

    union dwc_host_channel_transfer transfer_reg;
    union dwc_host_channel_characteristics char_reg;
    union dwc_host_channel_split_control split_control;
    void *data;
    uint32_t next_frame;

    switch (dev_stat)
    {
        case POWERED:
            udelay(120*MILI_SEC);
            dwc_reset_host_port();
            printf("PORT:%x\t",regs->host_port_ctrlstatus);
            // Set address setup request
            setup_data.bmRequestType = USB_BMREQUESTTYPE_DIR_OUT | USB_REQUEST_TYPE_STANDARD
            | USB_REQUEST_RECIPIENT_DEVICE; // A length way to say zero, lel.
            setup_data.bRequest = USB_DEVICE_REQUEST_SET_ADDRESS;
            setup_data.wValue = KEYBOARD_DEVICE_ADDRESS; //0;
            setup_data.wIndex = setup_data.wLength = 0;

            transfer_reg.packet_id = 0x3;
            transfer_reg.packet_count = 1;
            transfer_reg.size = sizeof(setup_data);
            regs->host_channels[0].transfer.val = transfer_reg.val;
            data = &setup_data;
            regs->host_channels[0].dma_address = (uint32_t)data;

            // Enable channel 0
            regs->host_channels_interrupt_mask = 1;
            regs->host_channels[0].interrupt_mask.val = 0x3ff;

            // If device is not high speed we have TODO with this.
            split_control.val = 0;
            split_control.split_enable = 1;
            regs->host_channels[0].split_control.val = split_control.val;
            next_frame = (regs->host_frame_number & 0xffff) + 1;
            char_reg.max_packet_size = 8; // Idk
            char_reg.endpoint_type = USB_TRANSFER_TYPE_CONTROL;
            char_reg.device_address = 0;
            char_reg.endpoint_direction = setup_data.bmRequestType >> 7;
            char_reg.odd_frame = next_frame & 1;
            char_reg.packets_per_frame = 1;
            char_reg.endpoint_number = 0;
            // Starts the transaction.
            char_reg.channel_enable = 1;
            regs->host_channels[0].characteristics.val = char_reg.val;

            dev_stat = RESET;
            break;
        case RESET:
            printf("RESET\t");
            setup_data.bmRequestType = 0x21;
            setup_data.bRequest = 0x0b;
            setup_data.wValue = setup_data.wIndex = setup_data.wLength = 0;
            // Get device descriptor setup request
            /*setup_data.bmRequestType = USB_BMREQUESTTYPE_DIR_IN | USB_BMREQUESTTYPE_TYPE_STANDARD
            | USB_BMREQUESTTYPE_RECIPIENT_DEVICE;
            setup_data.bRequest = USB_DEVICE_REQUEST_GET_DESCRIPTOR;
            setup_data.wValue = USB_DESCRIPTOR_TYPE_DEVICE << 8 | 0;
            setup_data.wIndex = 0; setup_data.wLength = sizeof(descriptor);*/

            transfer_reg.packet_id = 0x3;
            transfer_reg.packet_count = 1;
            transfer_reg.size = sizeof(setup_data);
            regs->host_channels[0].transfer.val = transfer_reg.val;
            data = &setup_data;
            regs->host_channels[0].dma_address = (uint32_t)data;

            // Enable channel 0
            regs->host_channels_interrupt_mask = 1;
            regs->host_channels[0].interrupt_mask.val = 0x3ff;

            // If device is not high speed we have TODO with this.
            split_control.val = 0;
            split_control.split_enable = 1;
            regs->host_channels[0].split_control.val = split_control.val;
            next_frame = (regs->host_frame_number & 0xffff) + 1;
            char_reg.max_packet_size = 8; // Idk
            char_reg.endpoint_type = USB_TRANSFER_TYPE_CONTROL;
            char_reg.device_address = KEYBOARD_DEVICE_ADDRESS; //0;
            char_reg.endpoint_direction = setup_data.bmRequestType >> 7;
            char_reg.odd_frame = next_frame & 1;
            char_reg.packets_per_frame = 1;
            char_reg.endpoint_number = 0;
            // Starts the transaction.
            char_reg.channel_enable = 1;
            regs->host_channels[0].characteristics.val = char_reg.val;

            dev_stat = DEVICE_STATUS_COUNT;
            break;
        case CONFIGURED:
            printf("CONFIGURED\t");

            // Get device descriptor data phase
            transfer_reg.packet_id = 0x2;
            transfer_reg.packet_count = 3;
            transfer_reg.size = sizeof(descriptor);
            regs->host_channels[0].transfer.val = transfer_reg.val;
            data = &descriptor;
            regs->host_channels[0].dma_address = (uint32_t)data;

            // Enable channel 0
            regs->host_channels_interrupt_mask = 1;
            regs->host_channels[0].interrupt_mask.val = 0x3ff;

            // If device is not high speed we have TODO with this.
            split_control.val = 0;
            regs->host_channels[0].split_control.val = split_control.val;
            next_frame = (regs->host_frame_number & 0xffff) + 1;
            char_reg.max_packet_size = 8; // Idk
            char_reg.endpoint_type = USB_TRANSFER_TYPE_CONTROL;
            char_reg.device_address = 0;//KEYBOARD_DEVICE_ADDRESS;
            char_reg.endpoint_direction = setup_data.bmRequestType >> 7;
            char_reg.odd_frame = next_frame & 1;
            char_reg.packets_per_frame = 1;
            char_reg.endpoint_number = 0;
            // Starts the transaction.
            char_reg.channel_enable = 1;
            regs->host_channels[0].characteristics.val = char_reg.val;

            dev_stat = DEVICE_STATUS_COUNT;
            break;
        default:
            printf("DEFAULT\t");
            for(uint32_t i = 0; i < 8; ++i){
                printf("%d:%x ",i,key_press_data[i]);
            }
            printf("\t");
            /*printf("bLength:%x\tbDescriptorType:%x\tbcdUSB:%x\tbNumConfig:%x\t",
            descriptor.bLength, descriptor.bDescriptorType, descriptor.bcdUSB,
            descriptor.bNumConfigurations);*/
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

    if(regs->core_interrupts.host_channel_intr){
        host_channel_int = regs->host_channels[0].interrupts;
        /* Clear pending interrupts.  */
        regs->host_channels[0].interrupt_mask.val = 0;
        regs->host_channels[0].interrupts.val = 0xffffffff;
        printf("CHN:%x\t", host_channel_int.val);
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
    printf("1\t");
    if(!ON_EMU)
        bcm2835_setpower(POWER_USB, 1);
    printf("2\t");
    dwc_soft_reset();
    printf("3\t");
    dwc_setup_dma_mode();
    printf("4\t");
    dwc_power_on_host_port();
    dwc_setup_interrupts();
    printf("5\t");
    ENABLE_INTERRUPTS();
}

void usb_poll(void){
    union dwc_host_channel_transfer transfer_reg;
    union dwc_host_channel_characteristics char_reg;
    union dwc_host_channel_split_control split_control;
    void *data;
    uint32_t next_frame;
    if (dev_stat == DEVICE_STATUS_COUNT){
        // In without data, set device address accordingly, endpoint is 1
        transfer_reg.packet_id = 0x2;//0x3;
        transfer_reg.packet_count = 1;
        transfer_reg.size = sizeof(setup_data);
        regs->host_channels[0].transfer.val = transfer_reg.val;
        data = &key_press_data;//&setup_data;
        regs->host_channels[0].dma_address = (uint32_t)data;

        // Enable channel 0
        regs->host_channels_interrupt_mask = 1;
        regs->host_channels[0].interrupt_mask.val = 0x3ff;

        // If device is not high speed we have TODO with this.
        split_control.val = 0;
        split_control.split_enable = 1;
        regs->host_channels[0].split_control.val = split_control.val;
        next_frame = (regs->host_frame_number & 0xffff) + 1;
        char_reg.max_packet_size = 8; // Idk
        char_reg.endpoint_type = USB_TRANSFER_TYPE_INTERRUPT; //USB_TRANSFER_TYPE_CONTROL;
        char_reg.device_address = KEYBOARD_DEVICE_ADDRESS; //0;
        char_reg.endpoint_direction = USB_DIRECTION_IN;
        char_reg.odd_frame = next_frame & 1;
        char_reg.packets_per_frame = 1;
        char_reg.endpoint_number = 1; //0;

        // Starts the transaction.
        char_reg.channel_enable = 1;
        regs->host_channels[0].characteristics.val = char_reg.val;        
    }

}