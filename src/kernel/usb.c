#include <kernel/usb.h>
#include <kernel/usb_regs.h>
#include <kernel/interrupts.h>
#include <kernel/kerio.h>

uint8_t kbd_data[8] = {};
uint8_t usb_endpoint[24] = {};

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

/**
 * Array that holds pointers to the USB transfer request (if any) currently
 * being completed on each hardware channel.
 */
static struct usb_xfer_request *channel_pending_xfers[DWC_NUM_CHANNELS];

static volatile struct dwc_regs * const regs = (void*)USB_BASE;

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
/*
static void dwc_handle_channel_halted_interrupt(uint32_t chan){
    struct usb_xfer_request *req = channel_pending_xfers[chan];
    volatile struct dwc_host_channel *chanptr = &regs->host_channels[chan];
    union dwc_host_channel_interrupts interrupts = chanptr->interrupts;
    enum dwc_intr_status intr_status;

    printf("Handling channel %u halted interrupt\n"
              "\t\t(interrupts pending: 0x%08x, characteristics=0x%08x, "
              "transfer=0x%08x)\n",
              chan, interrupts.val, chanptr->characteristics.val,
              chanptr->transfer.val);

    // No apparent error occurred.
    intr_status = dwc_handle_normal_channel_halted(req, chan, interrupts);//TODO   

#if START_SPLIT_INTR_TRANSFERS_ON_SOF
    if ((intr_status == XFER_NEEDS_RESTART ||
         intr_status == XFER_NEEDS_TRANS_RESTART) &&
        usb_is_interrupt_request(req) && req->dev->speed != USB_SPEED_HIGH &&
        !req->complete_split)
    {
        intr_status = XFER_NEEDS_DEFERRAL;
        req->need_sof = 1;
    }
#endif

    req->status = USB_STATUS_SUCCESS;

    // Save the data packet ID.  
    req->next_data_pid = chanptr->transfer.packet_id;

    // Clear and disable interrupts on this channel.  
    chanptr->interrupt_mask.val = 0;
    chanptr->interrupts.val = 0xffffffff;

    // Release the channel.  
    channel_pending_xfers[chan] = NULL;
    dwc_release_channel(chan); //TODO         

    // Set the actual transferred size, unless we are doing a control transfer
    // and aren't on the DATA phase.  
    if (!usb_is_control_request(req) || req->control_phase == 1)
    {
        req->actual_size = req->cur_data_ptr - req->recvbuf;
    }
    char uint8_t c = req->recvbuf;
    printf("data size: %d", req->actual_size);
}

static void dwc_interrupt_handler(void)
{
    DISABLE_INTERRUPTS();
    union dwc_core_interrupts interrupts = regs->core_interrupts;

#if START_SPLIT_INTR_TRANSFERS_ON_SOF
    if (interrupts.sof_intr)
    {
        // Start of frame (SOF) interrupt occurred.

        printf("Received SOF intr (host_frame_number=0x%08x)\n",
                  regs->host_frame_number);
        if ((regs->host_frame_number & 0x7) != 6)
        {
            union dwc_core_interrupts tmp;

            if (sofwait != 0)
            {
                uint chan;

                // Wake up one channel waiting for SOF 

                chan = first_set_bit(sofwait);
                send(channel_pending_xfers[chan]->deferer_thread_tid, 0);
                sofwait &= ~(1 << chan);
            }

            // Disable SOF interrupt if no longer needed 
            if (sofwait == 0)
            {
                tmp = regs->core_interrupt_mask;
                tmp.sof_intr = 0;
                regs->core_interrupt_mask = tmp;
            }

            // Clear SOF interrupt 
            tmp.val = 0;
            tmp.sof_intr = 1;
            regs->core_interrupts = tmp;
        }
    }
#endif // START_SPLIT_INTR_TRANSFERS_ON_SOF

    if (interrupts.host_channel_intr)
    {
        // One or more channels has an interrupt pending.

        uint32_t chintr;
        uint chan;

        // A bit in the "Host All Channels Interrupt Register" is set if an
        // interrupt has occurred on the corresponding host channel.  Process
        // all set bits.
        chintr = regs->host_channels_interrupt;
        do
        {
            chan = first_set_bit(chintr);
            dwc_handle_channel_halted_interrupt(chan);
            chintr ^= (1 << chan);
        } while (chintr != 0);
    }
    if (interrupts.port_intr)
    {
        // Status of the host port changed.  Update host_port_status.

        union dwc_host_port_ctrlstatus hw_status = regs->host_port_ctrlstatus;

        printf("Port interrupt detected: host_port_ctrlstatus=0x%08x\n",
                  hw_status.val);                 

        host_port_status.connected   = hw_status.connected;
        host_port_status.enabled     = hw_status.enabled;
        host_port_status.suspended   = hw_status.suspended;
        host_port_status.overcurrent = hw_status.overcurrent;
        host_port_status.reset       = hw_status.reset;
        host_port_status.powered     = hw_status.powered;
        host_port_status.low_speed_attached = (hw_status.speed == USB_SPEED_LOW);
        host_port_status.high_speed_attached = (hw_status.speed == USB_SPEED_HIGH);

        host_port_status.connected_changed   = hw_status.connected_changed;
        host_port_status.enabled_changed     = hw_status.enabled_changed;
        host_port_status.overcurrent_changed = hw_status.overcurrent_changed;

        // Clear the interrupt(s), which are "write-clear", by writing the Host
        // Port Control and Status register back to itself.  But as a special
        // case, 'enabled' must be written as 0; otherwise the port will
        // apparently disable itself.
        hw_status.enabled = 0;
        regs->host_port_ctrlstatus = hw_status;

        // Complete status change request to the root hub if one has been
        // submitted.
        dwc_host_port_status_changed(); //TODO
    }

    ENABLE_INTERRUPTS();
}
*/
static void usb_interrupt_handler(void){
    printf("USB HANDLER\tCORE INTR:%x\tHOST CHNLS: %x\tPORT STAT:%x\t",
    regs->core_interrupts.val, regs->host_channels_interrupt, regs->host_port_ctrlstatus.val);

    union dwc_core_interrupts interrupts = regs->core_interrupts;
    if(interrupts.sof_intr){
        printf("SOF INTR HOST FRAME NUM:%x\t", regs->host_frame_number);
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
            printf("CHAN %d\tTRNSFR VAL: %x\tCHRCTERSTCS:%x\tSC: %x\tINTRPS: %x\t",
            chan, regs->host_channels[chan].transfer.val,
            regs->host_channels[chan].characteristics.val,
            regs->host_channels[chan].split_control,
            regs->host_channels[chan].interrupts);
            chintr ^= (1 << chan);
        } while (chintr != 0);        
    }

    if (interrupts.port_intr){
        /* Status of the host port changed.  Update host_port_status.  */
        union dwc_host_port_ctrlstatus hw_status = regs->host_port_ctrlstatus;
        host_port_status.connected   = hw_status.connected;
        host_port_status.enabled     = hw_status.enabled;
        host_port_status.suspended   = hw_status.suspended;
        host_port_status.overcurrent = hw_status.overcurrent;
        host_port_status.reset       = hw_status.reset;
        host_port_status.powered     = hw_status.powered;
        host_port_status.speed = hw_status.speed;
        //host_port_status.low_speed_attached = (hw_status.speed == USB_SPEED_LOW);
        //host_port_status.high_speed_attached = (hw_status.speed == USB_SPEED_HIGH);

        host_port_status.connected_changed   = hw_status.connected_changed;
        host_port_status.enabled_changed     = hw_status.enabled_changed;
        host_port_status.overcurrent_changed = hw_status.overcurrent_changed;

        /* Clear the interrupt(s), which are "write-clear", by writing the Host
         * Port Control and Status register back to itself.  But as a special
         * case, 'enabled' must be written as 0; otherwise the port will
         * apparently disable itself.  */
        hw_status.enabled = 0;
        regs->host_port_ctrlstatus = hw_status;

        //TODO send data ?
        //regs->host_channels[0].characteristics.max_packet_size = 64;
        //regs->host_channels[0].characteristics.endpoint_number = 0;
        //regs->host_channels[0].characteristics = USB_DIRECTION_IN;
        //regs->host_channels[0].characteristics.packets_per_frame = 1;
    }
}

static void usb_interrupt_clearer(void){
    printf("USB CLEARER\t");
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
    bcm2835_setpower(POWER_USB, 1);
    dwc_soft_reset();
    dwc_setup_dma_mode();
    dwc_setup_interrupts();
    //dwc_start_xfer_scheduler
}