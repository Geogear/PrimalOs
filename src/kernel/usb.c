#include <kernel/usb.h>
#include <kernel/usb_regs.h>
#include <kernel/interrupts.h>
#include <kernel/kerio.h>
#include <kernel/timer.h>
#include <kernel/usb_std_defs.h>
#include <kernel/keyboard.h>
#include <kernel/scancodes.h>
#include <kernel/mutex.h>
#include <common/stdlib.h>

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

static struct usb_control_setup_data setup_data = {};
static uint8_t transfer_buffer[512] = {};
static uint8_t line_buffer[256] = {};
static uint16_t cursor = 0;

static enum device_status dev_stat = DEVICE_STATUS_COUNT;
static uint8_t config_val = 0;
static uint8_t interface_num = 0;
static uint8_t dev_speed_low = 0;
static uint8_t control_phase = 0;
static uint8_t interrupt_phase = 0;
static uint8_t active_address = 0;
static uint8_t transfer_active = 0;
static uint8_t endpoint_address = 0;
static uint8_t endpoint_interval = 0;
static uint8_t interface_index = 0;
static uint8_t interrupt_poll_active = 0;
static uint32_t config_desc_len = sizeof(struct usb_configuration_descriptor);

static uint8_t prev_transfer = 255;
static uint8_t can_poll = 0;
static uint8_t enter_pressed = 0;

extern uint8_t ilock_active = 0;

static void buf_key_data_check(uint32_t x){
    printf("BKD_CHECK: %x\t",x);
    /*uint32_t total = 0;
    for(uint32_t i = 0; i < 8; ++i)
        total += transfer_buffer[i];
    
    if(total > 0){
        for(uint32_t i = 0; i < 8; ++i)
            printf("KD%d: %d\t", i, transfer_buffer[i]);  
    }else
        printf("%d",x);*/
    // convert scancodes to ascii values and store in the line buf
    // convert each key until the first non-printable character or data end
    //uint8_t input = 0;

    // If a line is read but not processed, don't further read any key press data.
    if(enter_pressed)
        return;

    struct key_press_report* krp = (struct key_press_report*)(&transfer_buffer[0]);
    for(uint32_t i = 0; i < 7; ++i){
        uint8_t ascii = get_ascii_from_sc(krp->key_press_data[i]);
        if(in_printable_range(ascii)){
            //input = 1;
            // Make sure line buffer doesn't overflow
            if(cursor < 256)
                line_buffer[cursor++] = ascii;
            if(ascii == LF)
                enter_pressed = 1;
            // Print the pressed character
            putc(ascii);
        }else
            break;
    }
}

static uint32_t round_up_divide(uint32_t dividend, uint32_t divider){
    uint32_t result = 0;
    while(dividend > divider){
        dividend -= divider;
        ++result;
    }
    if(dividend != 0)
        ++result;
    return result;
}

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
            //*printf("SETUP\t");
            char_reg.endpoint_direction = USB_DIRECTION_OUT;
            data = &setup_data;
            transfer_reg.size = sizeof(struct usb_control_setup_data);
            transfer_reg.packet_id = DWC_USB_PID_SETUP;
            control_phase = (setup_data.wLength == 0) ? 2 : 1;
            break;
        case 1:
            //data phase
            //*printf("DATA\t");
            char_reg.endpoint_direction = setup_data.bmRequestType >> 7;
            /* We need to carefully take into account that we might be
                 * re-starting a partially complete transfer.  */
            transfer_reg.size = setup_data.wLength;
            transfer_reg.packet_count = round_up_divide(transfer_reg.size,
            char_reg.max_packet_size);
            data = &transfer_buffer[0];
            //printf("DBUF: %x\tDMAADR: %x\tPCKTCNT: %x\t",
                //(uint32_t)(&transfer_buffer[0]), (uint32_t)data, transfer_reg.packet_count);
            control_phase = 2;
            transfer_reg.packet_id = DWC_USB_PID_DATA1;
            break;        
        default:
            // status phase
            //*printf("STATUS\t");
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
    if(transfer_reg.packet_count == 0)
        transfer_reg.packet_count = 1;
    char_reg.channel_enable = 1;

    /* Actually program the registers of the appropriate channel.  */
    regs->host_channels[0].transfer = transfer_reg;
    regs->host_channels[0].dma_address = (uint32_t)data;
    regs->host_channels[0].characteristics = char_reg;
}

static void usb_interrupt_handler(void){
    DISABLE_INTERRUPTS();
    //*printf("USB_HANDLER\t");
    //printf("CORE INTR:%x\tHOST CHNLS: %x\tPORT STAT:%x\tCHAN 0 INT:%x\t",
    //regs->core_interrupts.val, regs->host_channels_interrupt,
    //regs->host_port_ctrlstatus.val, regs->host_channels[0].interrupt_mask.val);
    uint8_t prev_phase = 0;
    struct usb_configuration_descriptor* config_desc;
    struct usb_device_descriptor* dev_desc;

    union dwc_core_interrupts interrupts = regs->core_interrupts;
    if(interrupts.sof_intr){
        /* Start of frame (SOF) interrupt occurred.  */
        if ((regs->host_frame_number & 0x7) != 6){
            union dwc_core_interrupts tmp;
             /* Disable SOF interrupt, not needed */
            tmp = regs->core_interrupt_mask;
            tmp.sof_intr = 0;
            regs->core_interrupt_mask = tmp;
            /* Clear SOF interrupt */
            tmp.val = 0;
            tmp.sof_intr = 1;
            regs->core_interrupts = tmp;            
        }
    }

    if(transfer_active){
        switch (dev_stat)
        {
            case POWERED:
                udelay(120*MILI_SEC);
                dwc_reset_host_port();
                //*printf("POW-PORT:%x\t",regs->host_port_ctrlstatus);
                //*printf("SPED:%d\t",host_port_status.speed);
                dev_speed_low = (host_port_status.speed == USB_SPEED_LOW) ? 1 : 0;
                dev_stat = RESET;
                break;
            case RESET:
                //*printf("RESET\t");
                // Set address setup request
                setup_data.bmRequestType = USB_BMREQUESTTYPE_DIR_OUT | USB_REQUEST_TYPE_STANDARD
                | USB_REQUEST_RECIPIENT_DEVICE; // A length way to say zero, lel.
                setup_data.bRequest = USB_DEVICE_REQUEST_SET_ADDRESS;
                setup_data.wValue = KEYBOARD_DEVICE_ADDRESS;
                setup_data.wIndex = setup_data.wLength = 0;

                // Enable channel 0
                regs->host_channels_interrupt_mask = 1;
                regs->host_channels[0].interrupt_mask.val = 0x3ff;

                prev_phase = control_phase;
                control_transfer();
                if(prev_phase == 2 && control_phase == 0){
                    active_address = KEYBOARD_DEVICE_ADDRESS;
                    dev_stat = ADDRESSED;
                    transfer_active = 1;
                    udelay(100*MILI_SEC); //Give the device recovery time.
                }
                break;
            case ADDRESSED:
                //*printf("ADDRESSED\t");
                // Enable channel interrupts
                regs->host_channels[0].interrupt_mask.val = 0x3ff;

                prev_phase = control_phase;
                control_transfer();
                if(prev_phase == 2 && control_phase == 0){
                    transfer_active = 0;
                    can_poll = 1;
                }
                break;
            case CONFIGURED:
                //*printf("CONFIGURED\t");
                switch (interrupt_phase)
                {
                    case 1:
                        key_poll(0);
                        break;
                    case 2:
                        buf_key_data_check(21);
                        transfer_active = 0;
                        interrupt_phase = 0;
                        break;
                }
                break;
            default:
                //*printf("DEFAULT\t");
                break;
        }
    }else if(transfer_active == 0 && prev_transfer != 255){
        switch (prev_transfer)
        {
            case 0: // get_status
                //*printf("PREV_TRANSFER: %d\t", prev_transfer);
                break;
            case 1: // get config
                //*printf("GET_CONFIG: %x\t", transfer_buffer[0]);
                break;
            case 2: // set config
                //*printf("PREV_TRANSFER: %d\t", prev_transfer);
                break;
            case 3: // get report, hdi specific
                buf_key_data_check(11);
                break;
            case 4: // get config desc
                config_desc = (struct usb_configuration_descriptor*)(&transfer_buffer[0]);
                config_desc_len = config_desc->wTotalLength;
                config_val = config_desc->bConfigurationValue;
                interface_num = config_desc->bNumInterfaces;
                //*printf("CONFIG DESC LENGLTH: %x, DESCRPTR_TYPE: %x, TOTAL_LEN: %x, NUM_INTRFCS: %x, CONFIG_VAL: %x,"
                //" ICONFIG: %x, ATTRBTS: %x, MAX_POWER: %x\t",
                //config_desc->bLength, config_desc->bDescriptorType, config_desc->wTotalLength, config_desc->bNumInterfaces,
                //config_desc->bConfigurationValue, config_desc->iConfiguration, config_desc->bmAttributes, config_desc->bMaxPower);
                break;
            case 5: // get protocol, hdi specific
                //*printf("GET_PROTOCOL: %x\t", transfer_buffer[0]);
                break;
            case 6: // set protocol, hdi specific
                //*printf("PREV_TRANSFER: %d\t", prev_transfer);
                break;
            case 7: //get device descriptor
                dev_desc = (struct usb_device_descriptor*)(&transfer_buffer[0]);
                //*printf("DEV DESCRPTR, LEN: %d\tCONF_NUM: %d\tBCD_USB: %x\t", 
                //dev_desc->bLength, dev_desc->bNumConfigurations, dev_desc->bcdUSB);
                break;
            default:
                printf("ERR: UNEXPECTED VAL %d PREV_TRANSFER.\t", prev_transfer);
                break;
        }
        prev_transfer = 255;
    }else{
        printf("ERR: UNEXPECTED LOGIC.\t");
    }

    ENABLE_INTERRUPTS();
}

static void usb_interrupt_clearer(void){
    //*printf("USB CLEARER\tPORT STAT:%x\tCORE INTR:%x\t",regs->host_port_ctrlstatus,
    //*regs->core_interrupts.val);
    
    if (regs->core_interrupts.port_intr){
        //*printf("PORT INTR IN\t");
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
        transfer_active = 1;
    }else if(!host_port_status.connected){
        dev_stat = DEVICE_STATUS_COUNT;
        transfer_active = 0;
        active_address = 0;
        interrupt_poll_active = 0;
    }

    host_channel_int = regs->host_channels[0].interrupts;
    //*printf("CHNL:%x\t", host_channel_int.val);
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
    //mutex_init(&input_lock);
    ilock_active = 1;
    ENABLE_INTERRUPTS();
}

int usb_poll(uint8_t request_type){
    int ret_val = -1;
    if (can_poll){
        //*printf("INPOL\t");
        prev_transfer = request_type;
        can_poll = 0;
        control_phase = 0;
        transfer_active = 1;       

        switch (request_type)
        {
            case 0: // get_status
                break;
            case 1: // get config
                setup_data.bmRequestType = USB_BMREQUESTTYPE_DIR_IN; 
                setup_data.bRequest = USB_DEVICE_REQUEST_GET_CONFIGURATION;
                setup_data.wValue = setup_data.wIndex = 0;
                setup_data.wLength = 1;
                break;
            case 2: // set config
                setup_data.bmRequestType = 0;
                setup_data.bRequest = USB_DEVICE_REQUEST_SET_CONFIGURATION;
                setup_data.wValue = config_val;
                setup_data.wIndex = setup_data.wLength = 0;
                break;
            case 3: // get report, hdi specific
                setup_data.bmRequestType = 0xa1;
                setup_data.bRequest = 0x01;
                setup_data.wValue = 0x0100;
                setup_data.wIndex = 0x0;
                setup_data.wLength = sizeof(struct key_press_report);
                break;
            case 4: // get config desc
                setup_data.bmRequestType = USB_BMREQUESTTYPE_DIR_IN;
                setup_data.bRequest = USB_DEVICE_REQUEST_GET_DESCRIPTOR;
                setup_data.wValue = USB_DESCRIPTOR_TYPE_CONFIGURATION << 8;
                setup_data.wIndex = 0;
                setup_data.wLength = config_desc_len;
                break;
            case 5: // get protocol, hdi specific
                setup_data.bmRequestType = 0xa1;
                setup_data.bRequest = 0x03;
                setup_data.wValue = 0;
                setup_data.wIndex = 0;
                setup_data.wLength = 1;         
                break;
            case 6: // set protocol, hdi specific
                setup_data.bmRequestType = 0x21;
                setup_data.bRequest = 0x0b;
                setup_data.wValue = 0; // 0 for boot, 1 for report
                setup_data.wIndex = 0;
                setup_data.wLength = 0;
                break;
            case 7: //get device descriptor
                setup_data.bmRequestType = USB_BMREQUESTTYPE_DIR_IN;
                setup_data.bRequest = USB_DEVICE_REQUEST_GET_DESCRIPTOR;
                setup_data.wValue = USB_DESCRIPTOR_TYPE_DEVICE << 8;
                setup_data.wIndex = 0;
                setup_data.wLength = sizeof(struct usb_device_descriptor);
                break;
            default:
                printf("ERR: UNEXPECTED VAL %d for REQUEST_TYPE.\t", request_type);
                break;
        }

        // Enable channel interrupts
        regs->host_channels[0].interrupt_mask.val = 0x3ff;
        control_transfer();
        ret_val = 0;
    }
    return ret_val;
}

static inline void poll_wait(uint32_t request_type, uint32_t milisecs){
    while(usb_poll(request_type) == -1){
        udelay(milisecs*MILI_SEC);
    }
}

void keyboard_enum(void){
    printf("\n--- GET DEV DESCRPTR ---\n");
    poll_wait(7, 200);
    bzero(&transfer_buffer[0], 512);

    printf("\n--- GET CFG DESCRPTR 0 ---\n");
    poll_wait(4, 200);

    udelay(2*SEC);
    struct usb_interface_descriptor* uid = NULL;

    printf("\n--- GET CFG DESCRPTR 0 ---\n");
    poll_wait(4, 200);
    uint32_t offset = sizeof(struct usb_configuration_descriptor);
    uint8_t exit = 0;

    udelay(2*SEC);
    for(uint32_t i = 0; i < interface_num; ++i){
        uid = (struct usb_interface_descriptor*)(&transfer_buffer[offset]);
        //*printf("\n--- INTERFACE %d ---\n", i);
        //*printf("INTERFACE DESCRPTR, LEN: %x, DSCRPTR_TYPE: %x, INTRFC_NUM: %x,"
        //"ALTERNATE_STTNG: %x, NUM_ENDPOINTS: %x, INTRFC_CLASS: %x,"
        //"INTRFC_SUBCLSS: %X, INTRFC_PRTCL: %x, INTRFC: %x",
        //uid->bLength, uid->bDescriptorType, uid->bInterfaceNumber,
        //uid->bAlternateSetting, uid->bNumEndpoints, uid->bInterfaceClass,
        //uid->bInterfaceSubClass, uid->bInterfaceProtocol, uid->iInterface);
        uint8_t epn = uid->bNumEndpoints;
        uint8_t cur_interface = uid->bInterfaceNumber;
        offset += sizeof(struct usb_interface_descriptor);

        struct hid_descriptor* hd = (struct hid_descriptor*)(&transfer_buffer[offset]);
        //*printf("\n--- INTERFACE %d HID DESCRIPTOR ---\n",i);
        //*printf("HID DESCRIPTOR, LEN: %x, DESCRPTR_TYPE: %x, bCD_HID: %x, bCNTRY_CODE: %x,"
        //" NUM_DESCRPTR: %x, DESCRPTR_TYPE_R: %x, DESCRPTR_LEN: %x", hd->bLength, hd->bDescriptorType,
        //hd->bcdHID, hd->bCountryCode, hd->bNumDescriptors, hd->bDescriptorTypeR, hd->wDescriptorLength);

        offset += sizeof(struct hid_descriptor);
        for(uint32_t j = 0; j < epn; ++j){
            struct usb_endpoint_descriptor* ued = (struct usb_endpoint_descriptor*)(&transfer_buffer[offset]);
            //*printf("\n--- INTERFACE %d ENPOINT %d ---\n", i, j);
            //*printf("EPOINT DESCRPTR, LEN: %x, DESCRPTR_TYPE: %x, EPOINT_ADRS: %x, "
            //"ATTRBTS: %x, MAX_PKT_SIZE: %x, bINTRVL: %x", ued->bLength, ued->bDescriptorType,
            //ued->bEndpointAddress, ued->bmAttributes, ued->wMaxPacketSize, ued->bInterval);
            
            if(((ued->bmAttributes & USB_TRANSFER_TYPE_INTERRUPT) == USB_TRANSFER_TYPE_INTERRUPT)
            && ((ued->bEndpointAddress >> 7) == USB_DIRECTION_IN)){
                printf("...ENTERED...\t");
                endpoint_address = ued->bEndpointAddress;
                endpoint_interval = ued->bInterval;
                interface_index = cur_interface;
                exit = 1;
                break;
            }
            offset += sizeof(struct usb_endpoint_descriptor);
        }
        if(exit)
            break;
    }

    if(exit == 0){
        printf("\nERROR: INTERRUPT IN ENDPOINT HASN'T BEEN FOUND.\n");
    }

    // Set config to config_val
    poll_wait(2, 200);
    udelay(2*SEC);
    dev_stat = CONFIGURED;
    interrupt_poll_active = 1;
    interrupt_phase = 0;
}

void key_poll(uint8_t may_skip){
    if((interrupt_poll_active == 0 || transfer_active) && may_skip)
        return;
    for(uint32_t i = 0; i < 8; ++i)
        transfer_buffer[i] = 0;
    transfer_active = 1;
    //*printf("KEYPOLL\t");
    union dwc_host_channel_characteristics char_reg = regs->host_channels[0].characteristics;
    dmb();
    union dwc_host_channel_transfer transfer_reg = regs->host_channels[0].transfer;
    char_reg.val = transfer_reg.val = 0;
    void* data = NULL;

    switch (interrupt_phase)
    {
        case 0:
            setup_data.bmRequestType = 0xa1;
            setup_data.bRequest = 0x01;
            setup_data.wValue = 0x0100;
            setup_data.wIndex = interface_index;
            setup_data.wLength = 8;
            data = &setup_data;
            char_reg.endpoint_direction = USB_DIRECTION_OUT;
            transfer_reg.packet_id = 0x0;
            break;
        case 1:
            char_reg.endpoint_direction = USB_DIRECTION_IN;
            data = &transfer_buffer[0];
            break;
    }

    char_reg.max_packet_size = 8;
    char_reg.endpoint_number = endpoint_address & 0x0f;
    //*printf("EPADRESS: %x, EDPNUM:%x\t",endpoint_address, char_reg.endpoint_number);
    char_reg.low_speed = dev_speed_low;
    char_reg.endpoint_type = USB_TRANSFER_TYPE_INTERRUPT;
    char_reg.packets_per_frame = 1;
    char_reg.device_address = active_address;
    char_reg.odd_frame = ((regs->host_frame_number & 0xffff) + 1) & 1;
    regs->host_channels[0].characteristics = char_reg;

    // Enable channel interrupts
    regs->host_channels[0].interrupt_mask.val = 0x3ff;

    transfer_reg.size = 8;
    transfer_reg.packet_count = 1;
    regs->host_channels[0].transfer = transfer_reg;
    dmb();
    regs->host_channels[0].dma_address = (uint32_t)data;

    ++interrupt_phase;
    char_reg.channel_enable = 1;
    regs->host_channels[0].characteristics = char_reg;
}

void get_line(uint8_t* buf, uint32_t len){
    printf("GETLINE\t");
    enter_pressed = 0;
    cursor = 0;
    bzero(line_buffer, 256);
    while(!enter_pressed){
        printf("GLOOP\t");
        key_poll(1);
        udelay(endpoint_interval*MILI_SEC);
    }

    // From linebuffer to caller buffer
    uint32_t i = 0;
    for(; i < cursor; ++i){
        if(i < len){
            buf[i] = line_buffer[i];
        }else{
            --i;
            break;
        }
    }
    // Set '\0'
    buf[i] = 0;   
}