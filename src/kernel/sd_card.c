#include <kernel/sd_host_defs.h>
#include <kernel/sd_host_regs.h>
#include <kernel/interrupts.h>
#include <kernel/kerio.h>
#include <kernel/kernel.h>
#include <kernel/timer.h>
#include <kernel/util.h>
#include <common/stdlib.h>

#define TIMEOUT_WAIT(stop_if_true, usecs) 		\
do {							\
	uint32_t curr = timer_get();	\
    uint32_t x;	\
	do						\
	{						\
		if(stop_if_true)			\
			break;				\
        x = timer_get() - curr;			\
	} while(x < usecs);			\
} while(0);

//TODO convert udelays to timeout_waits especially the 1 second ones.

static volatile struct emmc_regs * const host_regs = (void*)SD_REGS_BASE;
static union reg_interrupt interrupt_status = {};
static struct emmc_block_dev device = {};

static char *sd_versions[] = { "unknown", "1.0 and 1.01", "1.10",
    "2.00", "3.0x", "4.xx" };

static uint32_t sd_commands[] = {
    SD_CMD_INDEX(0),
    SD_CMD_RESERVED(1),
    SD_CMD_INDEX(2) | SD_RESP_R2,
    SD_CMD_INDEX(3) | SD_RESP_R6,
    SD_CMD_INDEX(4),
    SD_CMD_INDEX(5) | SD_RESP_R4,
    SD_CMD_INDEX(6) | SD_RESP_R1,
    SD_CMD_INDEX(7) | SD_RESP_R1b,
    SD_CMD_INDEX(8) | SD_RESP_R7,
    SD_CMD_INDEX(9) | SD_RESP_R2,
    SD_CMD_INDEX(10) | SD_RESP_R2,
    SD_CMD_INDEX(11) | SD_RESP_R1,
    SD_CMD_INDEX(12) | SD_RESP_R1b | SD_CMD_TYPE_ABORT,
    SD_CMD_INDEX(13) | SD_RESP_R1,
    SD_CMD_RESERVED(14),
    SD_CMD_INDEX(15),
    SD_CMD_INDEX(16) | SD_RESP_R1,
    SD_CMD_INDEX(17) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_INDEX(18) | SD_RESP_R1 | SD_DATA_READ | SD_CMD_MULTI_BLOCK | SD_CMD_BLKCNT_EN,
    SD_CMD_INDEX(19) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_INDEX(20) | SD_RESP_R1b,
    SD_CMD_RESERVED(21),
    SD_CMD_RESERVED(22),
    SD_CMD_INDEX(23) | SD_RESP_R1,
    SD_CMD_INDEX(24) | SD_RESP_R1 | SD_DATA_WRITE,
    SD_CMD_INDEX(25) | SD_RESP_R1 | SD_DATA_WRITE | SD_CMD_MULTI_BLOCK | SD_CMD_BLKCNT_EN,
    SD_CMD_RESERVED(26),
    SD_CMD_INDEX(27) | SD_RESP_R1 | SD_DATA_WRITE,
    SD_CMD_INDEX(28) | SD_RESP_R1b,
    SD_CMD_INDEX(29) | SD_RESP_R1b,
    SD_CMD_INDEX(30) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_RESERVED(31),
    SD_CMD_INDEX(32) | SD_RESP_R1,
    SD_CMD_INDEX(33) | SD_RESP_R1,
    SD_CMD_RESERVED(34),
    SD_CMD_RESERVED(35),
    SD_CMD_RESERVED(36),
    SD_CMD_RESERVED(37),
    SD_CMD_INDEX(38) | SD_RESP_R1b,
    SD_CMD_RESERVED(39),
    SD_CMD_RESERVED(40),
    SD_CMD_RESERVED(41),
    SD_CMD_RESERVED(42) | SD_RESP_R1,
    SD_CMD_RESERVED(43),
    SD_CMD_RESERVED(44),
    SD_CMD_RESERVED(45),
    SD_CMD_RESERVED(46),
    SD_CMD_RESERVED(47),
    SD_CMD_RESERVED(48),
    SD_CMD_RESERVED(49),
    SD_CMD_RESERVED(50),
    SD_CMD_RESERVED(51),
    SD_CMD_RESERVED(52),
    SD_CMD_RESERVED(53),
    SD_CMD_RESERVED(54),
    SD_CMD_INDEX(55) | SD_RESP_R1,
    SD_CMD_INDEX(56) | SD_RESP_R1 | SD_CMD_ISDATA,
    SD_CMD_RESERVED(57),
    SD_CMD_RESERVED(58),
    SD_CMD_RESERVED(59),
    SD_CMD_RESERVED(60),
    SD_CMD_RESERVED(61),
    SD_CMD_RESERVED(62),
    SD_CMD_RESERVED(63)
};

static uint32_t sd_acommands[] = {
    SD_CMD_RESERVED(0),
    SD_CMD_RESERVED(1),
    SD_CMD_RESERVED(2),
    SD_CMD_RESERVED(3),
    SD_CMD_RESERVED(4),
    SD_CMD_RESERVED(5),
    SD_CMD_INDEX(6) | SD_RESP_R1,
    SD_CMD_RESERVED(7),
    SD_CMD_RESERVED(8),
    SD_CMD_RESERVED(9),
    SD_CMD_RESERVED(10),
    SD_CMD_RESERVED(11),
    SD_CMD_RESERVED(12),
    SD_CMD_INDEX(13) | SD_RESP_R1,
    SD_CMD_RESERVED(14),
    SD_CMD_RESERVED(15),
    SD_CMD_RESERVED(16),
    SD_CMD_RESERVED(17),
    SD_CMD_RESERVED(18),
    SD_CMD_RESERVED(19),
    SD_CMD_RESERVED(20),
    SD_CMD_RESERVED(21),
    SD_CMD_INDEX(22) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_INDEX(23) | SD_RESP_R1,
    SD_CMD_RESERVED(24),
    SD_CMD_RESERVED(25),
    SD_CMD_RESERVED(26),
    SD_CMD_RESERVED(27),
    SD_CMD_RESERVED(28),
    SD_CMD_RESERVED(29),
    SD_CMD_RESERVED(30),
    SD_CMD_RESERVED(31),
    SD_CMD_RESERVED(32),
    SD_CMD_RESERVED(33),
    SD_CMD_RESERVED(34),
    SD_CMD_RESERVED(35),
    SD_CMD_RESERVED(36),
    SD_CMD_RESERVED(37),
    SD_CMD_RESERVED(38),
    SD_CMD_RESERVED(39),
    SD_CMD_RESERVED(40),
    SD_CMD_INDEX(41) | SD_RESP_R3,
    SD_CMD_INDEX(42) | SD_RESP_R1,
    SD_CMD_RESERVED(43),
    SD_CMD_RESERVED(44),
    SD_CMD_RESERVED(45),
    SD_CMD_RESERVED(46),
    SD_CMD_RESERVED(47),
    SD_CMD_RESERVED(48),
    SD_CMD_RESERVED(49),
    SD_CMD_RESERVED(50),
    SD_CMD_INDEX(51) | SD_RESP_R1 | SD_DATA_READ,
    SD_CMD_RESERVED(52),
    SD_CMD_RESERVED(53),
    SD_CMD_RESERVED(54),
    SD_CMD_RESERVED(55),
    SD_CMD_RESERVED(56),
    SD_CMD_RESERVED(57),
    SD_CMD_RESERVED(58),
    SD_CMD_RESERVED(59),
    SD_CMD_RESERVED(60),
    SD_CMD_RESERVED(61),
    SD_CMD_RESERVED(62),
    SD_CMD_RESERVED(63)
};

// Reset the CMD line
static int sd_reset_cmd(void){
    union reg_control1 control1;
    control1 = host_regs->control1;
    control1.srst_cmd = 1;

    host_regs->control1 = control1;
    TIMEOUT_WAIT(host_regs->control1.srst_cmd == 0, SEC);
    control1 = host_regs->control1;
    if(!control1.srst_cmd){
        printf("EMMC: LINE DIDN'T RESET PROPERLY.\t");
        return -1;
    }
    return 0;
}

// Reset the Data line
static int sd_reset_dat(void){
    union reg_control1 control1;
    control1 = host_regs->control1;
    control1.srst_data = 1;

    host_regs->control1 = control1;
    TIMEOUT_WAIT(host_regs->control1.srst_data == 0, SEC);
    control1 = host_regs->control1;
    if(!control1.srst_data){
        printf("EMMC: LINE DIDN'T RESET PROPERLY.\t");
        return -1;
    }
    return 0;
}

static uint32_t sd_get_base_clock_hz()
{
    uint32_t base_clock = host_regs->not_def0[0];
    base_clock = ((base_clock >> 8) & 0xff) * 1000000;
    return base_clock;
}

// Set the clock dividers to generate a target value
static uint32_t sd_get_clock_divider(uint32_t base_clock, uint32_t target_rate)
{
    uint32_t targetted_divisor = 0;
    if(target_rate > base_clock)
        targetted_divisor = 1;
    else
    {
        targetted_divisor = base_clock / target_rate;
        uint32_t mod = base_clock % target_rate;
        if(mod)
            targetted_divisor--;
    }

    // HCI version 3 or greater supports 10-bit divided clock mode
    // This requires a power-of-two divider

    // Find the first bit set
    int divisor = -1;
    for(int first_bit = 31; first_bit >= 0; first_bit--)
    {
        uint32_t bit_test = (1 << first_bit);
        if(targetted_divisor & bit_test)
        {
            divisor = first_bit;
            targetted_divisor &= ~bit_test;
            if(targetted_divisor)
            {
                // The divisor is not a power-of-two, increase it
                divisor++;
            }
            break;
        }
    }

    if(divisor == -1)
        divisor = 31;
    if(divisor >= 32)
        divisor = 31;

    if(divisor != 0)
        divisor = (1 << (divisor - 1));

    if(divisor >= 0x400)
        divisor = 0x3ff;

    uint32_t freq_select = divisor & 0xff;
    uint32_t upper_bits = (divisor >> 8) & 0x3;
    uint32_t ret = (freq_select << 8) | (upper_bits << 6) | (0 << 5);
    
    return ret;
}

static int sd_switch_clock_rate(uint32_t base_clock, uint32_t target_rate){
    // Decide on an appropriate divider
    uint32_t divider = sd_get_clock_divider(base_clock, target_rate);
    if(divider == SD_GET_CLOCK_DIVIDER_FAIL)
    {
        printf("EMMC: couldn't get a valid divider for target rate %i Hz\n",
               target_rate);
        return -1;
    }

    // Wait for the command inhibit (CMD and DAT) bits to clear
    while(host_regs->status.val & 0x3)
        udelay(1000);

    // Set the SD clock off
    dmb();
    uint32_t control1 = host_regs->control1.val;
    control1 &= ~(1 << 2);
    host_regs->control1.val = control1;
    udelay(2000);

    // Write the new divider
	control1 &= ~0xffe0;		// Clear old setting + clock generator select
    control1 |= divider;
    host_regs->control1.val = control1;
    udelay(2000);

    // Enable the SD clock
    control1 |= (1 << 2);
    host_regs->control1.val = control1;
    udelay(2000);

#ifdef EMMC_DEBUG
    printf("EMMC: successfully set clock rate to %i Hz\n", target_rate);
#endif
    return 0;    
}

static void sd_issue_command_int(uint32_t cmd_reg, uint32_t argument, uint32_t timeout){
    device.last_cmd_reg = cmd_reg;
    device.last_cmd_success = 0;

    // Wait until cmd line is free
    while(host_regs->status.cmd_inhibit)
        udelay(1000);
    // Is the command with busy?
    if((cmd_reg & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B)
    {
        // With busy
        // Is is an abort command?
        if((cmd_reg & SD_CMD_TYPE_MASK) != SD_CMD_TYPE_ABORT)
        {
            // Not an abort command
            // Wait for the data line to be free
            while(host_regs->status.dat_inhibit)
                udelay(1000);
        }
    }

    // Is this a DMA transfer?
    uint32_t is_sdma = ((cmd_reg & SD_CMD_ISDATA) && (device.use_sdma));
    if(is_sdma)
    {
        // Set system address register (ARGUMENT2 in RPi)
        // We need to define a 4 kiB aligned buffer to use here
        // Then convert its virtual address to a bus address
        host_regs->arg2 = SDMA_BUFFER_PA;      
    }

    // Set block size and block count
    // For now, block size = 512 bytes, block count = 1,
    //  host SDMA buffer boundary = 4 kiB
    if(device.blocks_to_transfer > 0xffff)
    {
        printf("SD: BLOCKS_TO_TRANSFER TOO GREAT (%i)\t",
               device.blocks_to_transfer);
        device.last_cmd_success = 0;
        return;
    }    
    union reg_blk_size_cnt blk_size_cnt;
    blk_size_cnt.blk_size = device.block_size;
    blk_size_cnt.blk_cnt = device.blocks_to_transfer;
    host_regs->blk_size_cnt = blk_size_cnt;

    /// Set argument 1 reg
    dmb();
    host_regs->arg1 = argument;

    if(is_sdma)
    {
        // Set Transfer mode register
        cmd_reg |= SD_CMD_DMA;
    }

    // Set command reg
    host_regs->cmdtm.val = cmd_reg;
    udelay(2000);

    // Wait for command complete interrupt
    TIMEOUT_WAIT(host_regs->interrupt_flags.val & 0x8001, timeout);
    dmb();
    union reg_interrupt irpts = host_regs->interrupt_flags;

    // Clear command complete status
    host_regs->interrupt_flags.val = irpts.val & 0xffff0001;

    // Test for errors
    if((irpts.val & 0xffff0001) != 0x1)
    {
        printf("SD: ERROR OCCURED WHILST WAITING FOR COMMAND COMPLETE INTERRUPT.\t");
        device.last_error = irpts.val & 0xffff0000;
        device.last_interrupt = irpts.val;
        return;
    }
    udelay(2000);

    // Get response data
    switch(cmd_reg & SD_CMD_RSPNS_TYPE_MASK)
    {
        case SD_CMD_RSPNS_TYPE_48:
        case SD_CMD_RSPNS_TYPE_48B:
            device.last_r0 = host_regs->resp[0];
            break;
        case SD_CMD_RSPNS_TYPE_136:
            device.last_r0 = host_regs->resp[0];
            dmb();
            device.last_r1 = host_regs->resp[1];
            dmb();
            device.last_r2 = host_regs->resp[2];
            dmb();
            device.last_r3 = host_regs->resp[3];
            dmb();
            break;
    }

    // If with data, wait for the appropriate interrupt
    if((cmd_reg & SD_CMD_ISDATA) && (is_sdma == 0))
    {
        uint32_t wr_irpt;
        int is_write = 0;
        if(cmd_reg & SD_CMD_DAT_DIR_CH)
            wr_irpt = (1 << 5);     // read
        else
        {
            is_write = 1;
            wr_irpt = (1 << 4);     // write
        }

        int cur_block = 0;
        uint32_t *cur_buf_addr = (uint32_t *)device.buf;

        while(cur_block < device.blocks_to_transfer)
        {
			if(device.blocks_to_transfer > 1)
				printf("SD: MULTI BLOCK TRANSFER, AWAITING BLOCK %i READY.\t", cur_block);            
        
            TIMEOUT_WAIT(host_regs->interrupt_flags.val & (wr_irpt | 0x8000), timeout);
            irpts = host_regs->interrupt_flags;
            host_regs->interrupt_flags.val = 0xffff0000 | wr_irpt;

            if((irpts.val & (0xffff0000 | wr_irpt)) != wr_irpt)
            {
                printf("SD: ERROR OCCURED WHILST WAITING FOR DATA READY INTERRUPT.\t");
                device.last_error = irpts.val & 0xffff0000;
                device.last_interrupt = irpts.val;
                return;
            }

            // Transfer the block
            size_t cur_byte_no = 0;
            while(cur_byte_no < device.block_size)
            {
                if(is_write)
                {
                    uint32_t data = read_word((uint8_t *)cur_buf_addr, 0);
                    host_regs->data = data;
                }
                else
                {
                    uint32_t data = host_regs->data;
                    write_word(data, (uint8_t *)cur_buf_addr, 0);
                }
                cur_byte_no += 4;
                cur_buf_addr++;
            }
            ++cur_block;
        }
    }

    // Wait for transfer complete (set if read/write transfer or with busy)
    if((((cmd_reg & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B) ||
       (cmd_reg & SD_CMD_ISDATA)) && (is_sdma == 0))
    {
        // First check command inhibit (DAT) is not already 0
        if(!host_regs->status.dat_inhibit)
            host_regs->interrupt_flags.val = 0xffff0002;
        else
        {
            TIMEOUT_WAIT(host_regs->interrupt_flags.val & 0x8002, timeout);
            irpts = host_regs->interrupt_flags;
            host_regs->interrupt_flags.val = 0xffff0002;

            // Handle the case where both data timeout and transfer complete
            //  are set - transfer complete overrides data timeout: HCSS 2.2.17
            if(((irpts.val & 0xffff0002) != 0x2) && ((irpts.val & 0xffff0002) != 0x100002))
            {
                printf("SD: ERROR OCCURED WHILST WAITING FOR TRANSFER COMPLETE INTERRUPT.\t");
                device.last_error = irpts.val & 0xffff0000;
                device.last_interrupt = irpts.val;
                return;
            }
            host_regs->interrupt_flags.val = 0xffff0002;
        }
    }
    // TODO skipped sdma part for now.
    /*else if (is_sdma)
    {
        // For SDMA transfers, we have to wait for either transfer complete,
        //  DMA int or an error

        // First check command inhibit (DAT) is not already 0
        if((mmio_read(emmc_base + EMMC_STATUS) & 0x2) == 0)
            mmio_write(emmc_base + EMMC_INTERRUPT, 0xffff000a);
        else
        {
            TIMEOUT_WAIT(mmio_read(emmc_base + EMMC_INTERRUPT) & 0x800a, timeout);
            irpts = mmio_read(emmc_base + EMMC_INTERRUPT);
            mmio_write(emmc_base + EMMC_INTERRUPT, 0xffff000a);

            // Detect errors
            if((irpts & 0x8000) && ((irpts & 0x2) != 0x2))
            {
#ifdef EMMC_DEBUG
                printf("SD: error occured whilst waiting for transfer complete interrupt\n");
#endif
                device.last_error = irpts & 0xffff0000;
                device.last_interrupt = irpts;
                return;
            }

            // Detect DMA interrupt without transfer complete
            // Currently not supported - all block sizes should fit in the
            //  buffer
            if((irpts & 0x8) && ((irpts & 0x2) != 0x2))
            {
#ifdef EMMC_DEBUG
                printf("SD: error: DMA interrupt occured without transfer complete\n");
#endif
                device.last_error = irpts & 0xffff0000;
                device.last_interrupt = irpts;
                return;
            }

            // Detect transfer complete
            if(irpts & 0x2)
            {
#ifdef EMMC_DEBUG
                printf("SD: SDMA transfer complete");
#endif
                // Transfer the data to the user buffer
                memcpy(device.buf, (const void *)SDMA_BUFFER, device.block_size);
            }
            else
            {
                // Unknown error
#ifdef EMMC_DEBUG
                if(irpts == 0)
                    printf("SD: timeout waiting for SDMA transfer to complete\n");
                else
                    printf("SD: unknown SDMA transfer error\n");

                printf("SD: INTERRUPT: %08x, STATUS %08x\n", irpts,
                       mmio_read(emmc_base + EMMC_STATUS));
#endif

                if((irpts == 0) && ((mmio_read(emmc_base + EMMC_STATUS) & 0x3) == 0x2))
                {
                    // The data transfer is ongoing, we should attempt to stop
                    //  it
#ifdef EMMC_DEBUG
                    printf("SD: aborting transfer\n");
#endif
                    mmio_write(emmc_base + EMMC_CMDTM, sd_commands[STOP_TRANSMISSION]);

#ifdef EMMC_DEBUG
                    // pause to let us read the screen
                    usleep(2000000);
#endif
                }
                device.last_error = irpts & 0xffff0000;
                device.last_interrupt = irpts;
                return;
            }
        }
    }*/

    // Return success
    device.last_cmd_success = 1;    
}

static void sd_issue_command(uint32_t command, uint32_t argument, uint32_t timeout){
    // Now run the appropriate commands by calling sd_issue_command_int()
    if(command & IS_APP_CMD)
    {
        device.last_cmd = APP_CMD;
        uint32_t rca = 0;
        if(device.card_rca)
            rca = device.card_rca << 16;
        sd_issue_command_int(sd_commands[APP_CMD], rca, timeout);
        if(device.last_cmd_success)
        {
            device.last_cmd = command | IS_APP_CMD;
            sd_issue_command_int(sd_acommands[command], argument, timeout);
        }        
    }else{
        device.last_cmd = command;
        sd_issue_command_int(sd_commands[command], argument, timeout);
    }
}

static void sd_handle_card_interrupt(void)
{
    // Handle a card interrupt
    union reg_status status = host_regs->status;
    printf("SD: CARD INTERRUPT.\tSD: CONTROLLER STATUS:%08x\t",status.val);

    // Get the card status
    if(device.card_rca)
    {
        sd_issue_command_int(sd_commands[SEND_STATUS], device.card_rca << 16,
                         500000);
        if(FAIL(&device))
        {
#ifdef EMMC_DEBUG
            printf("SD: UNABLE TO GET CARD STATUS.\t");
#endif
        }
        else
        {
#ifdef EMMC_DEBUG
            printf("SD: CARD STATUS: %08x\t", device.last_r0);
#endif
        }
    }
    else
    {
#ifdef EMMC_DEBUG
        printf("SD: NO CARD CURRENTLY SELECTED.\t");
#endif
    }

}

static void sd_handle_interrupts(void)
{
    uint32_t irpts = host_regs->interrupt_flags.val;
    uint32_t reset_mask = 0;

    if(irpts & SD_COMMAND_COMPLETE)
    {
#ifdef EMMC_DEBUG
        printf("SD: spurious command complete interrupt\n");
#endif
        reset_mask |= SD_COMMAND_COMPLETE;
    }

    if(irpts & SD_TRANSFER_COMPLETE)
    {
#ifdef EMMC_DEBUG
        printf("SD: spurious transfer complete interrupt\n");
#endif
        reset_mask |= SD_TRANSFER_COMPLETE;
    }

    if(irpts & SD_BLOCK_GAP_EVENT)
    {
#ifdef EMMC_DEBUG
        printf("SD: spurious block gap event interrupt\n");
#endif
        reset_mask |= SD_BLOCK_GAP_EVENT;
    }

    if(irpts & SD_DMA_INTERRUPT)
    {
#ifdef EMMC_DEBUG
        printf("SD: spurious DMA interrupt\n");
#endif
        reset_mask |= SD_DMA_INTERRUPT;
    }

    if(irpts & SD_BUFFER_WRITE_READY)
    {
#ifdef EMMC_DEBUG
        printf("SD: spurious buffer write ready interrupt\n");
#endif
        reset_mask |= SD_BUFFER_WRITE_READY;
        sd_reset_dat();
    }

    if(irpts & SD_BUFFER_READ_READY)
    {
#ifdef EMMC_DEBUG
        printf("SD: spurious buffer read ready interrupt\n");
#endif
        reset_mask |= SD_BUFFER_READ_READY;
        sd_reset_dat();
    }

    if(irpts & SD_CARD_INSERTION)
    {
#ifdef EMMC_DEBUG
        printf("SD: card insertion detected\n");
#endif
        reset_mask |= SD_CARD_INSERTION;
    }

    if(irpts & SD_CARD_REMOVAL)
    {
#ifdef EMMC_DEBUG
        printf("SD: card removal detected\n");
#endif
        reset_mask |= SD_CARD_REMOVAL;
        device.card_removal = 1;
    }

    if(irpts & SD_CARD_INTERRUPT)
    {
#ifdef EMMC_DEBUG
        printf("SD: card interrupt detected\n");
#endif
        sd_handle_card_interrupt();
        reset_mask |= SD_CARD_INTERRUPT;
    }

    if(irpts & 0x8000)
    {
#ifdef EMMC_DEBUG
        printf("SD: spurious error interrupt: %08x\n", irpts);
#endif
        reset_mask |= 0xffff0000;
    }
    host_regs->interrupt_flags.val = reset_mask;
}

int sd_card_init(void){

    // Power the board.
    if(!ON_EMU)
        bcm2835_setpower(POWER_SD, 1);

    // Read the version number.
    uint32_t ver = host_regs->spi_int_spt;
    uint32_t vendor = ver >> 24;
    uint32_t sdversion = (ver >> 16) & 0xff;
	uint32_t slot_status = ver & 0xff;

	printf("EMMC: VENDOR %x, SDVER %x, SLOT_STAT %x\t",
    vendor, sdversion, slot_status);

    if(sdversion < 2){
        printf("EMMC: ONLY SDHCI VERSIONS >= 3.0 ARE SUPPPORTED.\t");
        return -1;
    }

    // Reset the controller.
    union reg_control1 control1 = host_regs->control1;
    control1.srst_hc = 1;
    // Disable clock.
    control1.clk_en = 0;
    control1.clk_intlen = 0;
    host_regs->control1 = control1;

    udelay(SEC);
    control1 = host_regs->control1;
	if((control1.val & (0x7 << 24)) != 0){
		printf("EMMC: CONTROLLER DIDN'T RESET PROPERLY.\t");
		return -1;
	}

	// Clear control2.
    host_regs->control2.val = 0;

    // Get base clock rate.
    uint32_t base_clock = sd_get_base_clock_hz();
	if(base_clock == 0)
	{
	    printf("EMMC: ASSUMING CLOCK RATE AS 100MHz.\t");
	    base_clock = 100000000;
	}

    // Enable clock.
    control1 = host_regs->control1;
    control1.clk_intlen = 1;

	// Set to identification frequency (400 kHz)
	uint32_t f_id = sd_get_clock_divider(base_clock, SD_CLOCK_ID);
    control1.val |= f_id;
    control1.val |= (7 << 16);		// data timeout = TMCLK * 2^10

    host_regs->control1 = control1;
    udelay(SEC);
    control1 = host_regs->control1;
    if(!control1.clk_stable){
		printf("EMMC: CONTROLLER'S CLOCK DID NOT STABILISE WITHIN 1 SECOND.\t");
		return -1;        
    }

    udelay(2000);
    // Enable the SD clock.
    control1 = host_regs->control1;
    control1.clk_en = 1;
    host_regs->control1 = control1;
    udelay(2000);

    // Mask off sending interrupts to the ARM
    host_regs->interrupt_enable.val = 0;
    // Reset interrupts
    dmb();
    host_regs->interrupt_flags.val = 0xffffffff;
    // Have all interrupts sent to the INTERRUPT register,
    uint32_t irpt_mask = 0xffffffff; //&(~(1 << 8));
    host_regs->interrupt_flags_mask.val = irpt_mask;
    udelay(2000);

    device.block_size = 512;
    //device.bd.read = sd_read; TODO
    //device.bd.write = sd_write;
    //device.bd.supports_multiple_block_read = 1;
    //device.bd.supports_multiple_block_write = 1;
    device.base_clock = base_clock;

    // Send CMD0 to go to the idle state
    sd_issue_command(GO_IDLE_STATE, 0, 500000);
	if(FAIL(device))
	{
        printf("SD: no CMD0 response\n");
        return -1;
	}

	// Send CMD8 to the card
	// Voltage supplied = 0x1 = 2.7-3.6V (standard)
	// Check pattern = 10101010b (as per PLSS 4.3.13) = 0xAA
	sd_issue_command(SEND_IF_COND, 0x1aa, 500000);
	int v2_later = 0;
	if(TIMEOUT(device))
        v2_later = 0;
    else if(CMD_TIMEOUT(device))
    {
        if(sd_reset_cmd() == -1)
            return -1;
        host_regs->interrupt_flags.val = SD_ERR_MASK_CMD_TIMEOUT;
        v2_later = 0;
    }
    else if(FAIL(device))
    {
        printf("SD: failure sending CMD8 (%08x)\n", device.last_interrupt);
        return -1;
    }
    else
    {
        if((device.last_r0 & 0xfff) != 0x1aa)
        {
            printf("SD: unusable card\n");
#ifdef EMMC_DEBUG
            printf("SD: CMD8 response %08x\n", device.last_r0);
#endif
            return -1;
        }
        else
            v2_later = 1;
    }

    // Here we are supposed to check the response to CMD5 (HCSS 3.6)
    // It only returns if the card is a SDIO card
    sd_issue_command(IO_SET_OP_COND, 0, 10000);
    if(!TIMEOUT(device))
    {
        if(CMD_TIMEOUT(device))
        {
            if(sd_reset_cmd() == -1)
                return -1;
            host_regs->interrupt_flags.val = SD_ERR_MASK_CMD_TIMEOUT;
        }
        else
        {
            printf("SD: SDIO card detected - not currently supported\n");
#ifdef EMMC_DEBUG
            printf("SD: CMD5 returned %08x\n", device.last_r0);
#endif
            return -1;
        }
    }

    // Call an inquiry ACMD41 (voltage window = 0) to get the OCR
    sd_issue_command(ACMD(41), 0, 500000);
    if(FAIL(device))
    {
        printf("SD: inquiry ACMD41 failed\n");
        return -1;
    }

	// Call initialization ACMD41
	int card_is_busy = 1;
	while(card_is_busy)
	{
	    uint32_t v2_flags = 0;
	    if(v2_later)
	    {
	        // Set SDHC support
	        v2_flags |= (1 << 30);
	        if(device.failed_voltage_switch)
                v2_flags |= (1 << 24);
        }

	    sd_issue_command(ACMD(41), 0x00ff8000 | v2_flags, 500000);
	    if(FAIL(device))
	    {
	        printf("SD: error issuing ACMD41\n");
	        return -1;
	    }

	    if((device.last_r0 >> 31) & 0x1)
	    {
	        // Initialization is complete
	        device.card_ocr = (device.last_r0 >> 8) & 0xffff;
	        device.card_supports_sdhc = (device.last_r0 >> 30) & 0x1;

	        if(!device.failed_voltage_switch)
                device.card_supports_18v = (device.last_r0 >> 24) & 0x1;
                
	        card_is_busy = 0;
	    }else{
            // Card is still busy
            udelay(500000);
        }
    }

#ifdef EMMC_DEBUG
	printf("SD: card identified: OCR: %04x, 1.8v support: %i, SDHC support: %i\n",
			device.card_ocr, device.card_supports_18v, device.card_supports_sdhc);
#endif

    // At this point, we know the card is definitely an SD card, so will definitely
	//  support SDR12 mode which runs at 25 MHz
    sd_switch_clock_rate(base_clock, SD_CLOCK_NORMAL);

	// A small wait before the voltage switch
	udelay(5000);

	// Switch to 1.8V mode if possible
	if(device.card_supports_18v)
	{
	    // As per HCSS 3.6.1
	    // Send VOLTAGE_SWITCH
	    sd_issue_command(VOLTAGE_SWITCH, 0, 500000);
	    if(FAIL(device))
	    {
#ifdef EMMC_DEBUG
            printf("SD: error issuing VOLTAGE_SWITCH\n");
#endif
	        device.failed_voltage_switch = 1;
			bcm2835_setpower(POWER_SD, 0);
	        return sd_card_init();
	    }

	    // Disable SD clock
	    control1 = host_regs->control1;
	    control1.val &= ~(1 << 2);
	    host_regs->control1 = control1;

	    // Check DAT[3:0]
	    union reg_status status = host_regs->status;
	    uint32_t dat30 = (status.val >> 20) & 0xf;
	    if(dat30 != 0)
	    {
#ifdef EMMC_DEBUG
            printf("SD: DAT[3:0] did not settle to 0\n");
#endif
	        device.failed_voltage_switch = 1;
			bcm2835_setpower(POWER_SD, 0);
	        return sd_card_init();
	    }

	    // Set 1.8V signal enable to 1
	    union reg_control0 control0 = host_regs->control0;
	    control0.val |= (1 << 8);
	    host_regs->control0 = control0;

	    // Wait 5 ms
	    udelay(5000);

	    // Check the 1.8V signal enable is set
	    control0 = host_regs->control0;
	    if(((control0.val >> 8) & 0x1) == 0)
	    {
#ifdef EMMC_DEBUG
            printf("SD: controller did not keep 1.8V signal enable high\n");
#endif
	        device.failed_voltage_switch = 1;
			bcm2835_setpower(POWER_SD, 0);
	        return sd_card_init();
	    }

	    // Re-enable the SD clock
	    control1 = host_regs->control1;
	    control1.val |= (1 << 2);
        host_regs->control1 = control1;

	    // Wait 1 ms
	    udelay(10000);

	    // Check DAT[3:0]
        status = host_regs->status;
	    dat30 = (status.val >> 20) & 0xf;
	    if(dat30 != 0xf)
	    {
#ifdef EMMC_DEBUG
            printf("SD: DAT[3:0] did not settle to 1111b (%01x)\n", dat30);
#endif
	        device.failed_voltage_switch = 1;
			bcm2835_setpower(POWER_SD, 0);
	        return sd_card_init();
	    }

#ifdef EMMC_DEBUG
        printf("SD: voltage switch complete\n");
#endif
	}    

	// Send CMD2 to get the cards CID
	sd_issue_command(ALL_SEND_CID, 0, 500000);
	if(FAIL(device))
	{
	    printf("SD: error sending ALL_SEND_CID\n");
	    return -1;
	}
	uint32_t card_cid_0 = device.last_r0;
	uint32_t card_cid_1 = device.last_r1;
	uint32_t card_cid_2 = device.last_r2;
	uint32_t card_cid_3 = device.last_r3;
#ifdef EMMC_DEBUG
	printf("SD: card CID: %08x%08x%08x%08x\n", card_cid_3, card_cid_2, card_cid_1, card_cid_0);
#endif    
    //TODO, make device_id a static array and set the cid bytes in it.
	//device.bd.device_id = (uint8_t *)dev_id;
	//device.bd.dev_id_len = 4 * sizeof(uint32_t);

	// Send CMD3 to enter the data state
	sd_issue_command(SEND_RELATIVE_ADDR, 0, 500000);
	if(FAIL(device))
    {
        printf("SD: error sending SEND_RELATIVE_ADDR\n");
        return -1;
    }
    uint32_t cmd3_resp = device.last_r0;

	device.card_rca = (cmd3_resp >> 16) & 0xffff;
	uint32_t crc_error = (cmd3_resp >> 15) & 0x1;
	uint32_t illegal_cmd = (cmd3_resp >> 14) & 0x1;
	uint32_t error = (cmd3_resp >> 13) & 0x1;
	uint32_t status = (cmd3_resp >> 9) & 0xf;
	uint32_t ready = (cmd3_resp >> 8) & 0x1;

	if(crc_error)
	{
		printf("SD: CRC error\n");
		return -1;
	}
	if(illegal_cmd)
	{
		printf("SD: illegal command\n");
		return -1;
	}
	if(error)
	{
		printf("SD: generic error\n");
		return -1;
	}
	if(!ready)
	{
		printf("SD: not ready for data\n");
		return -1;
	}

	// Now select the card (toggles it to transfer state)
	sd_issue_command(SELECT_CARD, device.card_rca << 16, 500000);
	if(FAIL(device))
	{
	    printf("SD: error sending CMD7\n");
	    return -1;
	}

	uint32_t cmd7_resp = device.last_r0;
	status = (cmd7_resp >> 9) & 0xf;

	if((status != 3) && (status != 4))
	{
		printf("SD: invalid status (%i)\n", status);
		return -1;
	}

	// If not an SDHC card, ensure BLOCKLEN is 512 bytes
	if(!device.card_supports_sdhc)
	{
	    sd_issue_command(SET_BLOCKLEN, 512, 500000);
	    if(FAIL(device))
	    {
	        printf("SD: error sending SET_BLOCKLEN\n");
	        return -1;
	    }
	}
	device.block_size = 512;
	uint32_t controller_block_size = host_regs->blk_size_cnt.val;
	controller_block_size &= (~0xfff);
	controller_block_size |= 0x200;
    host_regs->blk_size_cnt.val = controller_block_size;;

	// Get the cards SCR register
	device.buf = &device.scr.scr[0];
	device.block_size = 8;
	device.blocks_to_transfer = 1;
	sd_issue_command(SEND_SCR, 0, 500000);
	device.block_size = 512;
	if(FAIL(device))
	{
	    printf("SD: error sending SEND_SCR\n");
	    return -1;
	}

	// Determine card version
	// Note that the SCR is big-endian
	uint32_t scr0 = byte_swap(device.scr.scr[0]);
	device.scr.sd_version = SD_VER_UNKNOWN;
	uint32_t sd_spec = (scr0 >> (56 - 32)) & 0xf;
	uint32_t sd_spec3 = (scr0 >> (47 - 32)) & 0x1;
	uint32_t sd_spec4 = (scr0 >> (42 - 32)) & 0x1;
	device.scr.sd_bus_widths = (scr0 >> (48 - 32)) & 0xf;
	if(sd_spec == 0)
        device.scr.sd_version = SD_VER_1;
    else if(sd_spec == 1)
        device.scr.sd_version = SD_VER_1_1;
    else if(sd_spec == 2)
    {
        if(sd_spec3 == 0)
            device.scr.sd_version = SD_VER_2;
        else if(sd_spec3 == 1)
        {
            if(sd_spec4 == 0)
                device.scr.sd_version = SD_VER_3;
            else if(sd_spec4 == 1)
                device.scr.sd_version = SD_VER_4;
        }
    }

    if(device.scr.sd_bus_widths & 0x4)
    {
        // Set 4-bit transfer mode (ACMD6)
        // See HCSS 3.4 for the algorithm
#ifdef SD_4BIT_DATA

        // Disable card interrupt in host
        uint32_t old_irpt_mask = host_regs->interrupt_flags_mask.val;
        uint32_t new_iprt_mask = old_irpt_mask & ~(1 << 8);
        host_regs->interrupt_flags_mask.val = new_iprt_mask;

        // Send ACMD6 to change the card's bit mode
        sd_issue_command(SET_BUS_WIDTH, 0x2, 500000);
        if(FAIL(device))
            printf("SD: switch to 4-bit data mode failed\n");
        else
        {
            // Change bit mode for Host
            uint32_t control0 = host_regs->control0.val;
            control0 |= 0x2;
            host_regs->control0.val = control0;

            // Re-enable card interrupt in host
            host_regs->interrupt_flags_mask.val = old_irpt_mask;
        }
#endif
    }      

	printf("SD: found a valid version %s SD card\n", sd_versions[device.scr.sd_version]);
#ifdef EMMC_DEBUG
	printf("SD: setup successful (status %i)\n", status);
#endif

	// Reset interrupt register
	host_regs->interrupt_flags.val = 0xffffffff;    
    return 0;
}

static int sd_ensure_data_mode(void){
	if(device.card_rca == 0)
	{
		// Try again to initialise the card
		int ret = sd_card_init();
		if(ret != 0)
			return ret;
	}

    sd_issue_command(SEND_STATUS, device.card_rca << 16, 500000);
    if(FAIL(device))
    {
        printf("SD: ensure_data_mode() error sending CMD13\n");
        device.card_rca = 0;
        return -1;
    }

	uint32_t status = device.last_r0;
	uint32_t cur_state = (status >> 9) & 0xf;  
	if(cur_state == 3)
	{
		// Currently in the stand-by state - select it
		sd_issue_command(SELECT_CARD, device.card_rca << 16, 500000);
		if(FAIL(device))
		{
			printf("SD: ensure_data_mode() no response from CMD17\n");
			device.card_rca = 0;
			return -1;
		}
	}
	else if(cur_state == 5)
	{
		// In the data transfer state - cancel the transmission
		sd_issue_command(STOP_TRANSMISSION, 0, 500000);
		if(FAIL(device))
		{
			printf("SD: ensure_data_mode() no response from CMD12\n");
			device.card_rca = 0;
			return -1;
		}

		// Reset the data circuit
		sd_reset_dat();
	}
	else if(cur_state != 4)
	{
		// Not in the transfer state - re-initialise
		int ret = sd_card_init();
		if(ret != 0)
			return ret;
	}

	// Check again that we're now in the correct mode
	if(cur_state != 4)
	{
        sd_issue_command(SEND_STATUS, device.card_rca << 16, 500000);
        if(FAIL(device))
		{
			printf("SD: ensure_data_mode() no response from CMD13\n");
			device.card_rca = 0;
			return -1;
		}
		status = device.last_r0;
		cur_state = (status >> 9) & 0xf;

		if(cur_state != 4)
		{
			printf("SD: unable to initialise SD card to "
					"data mode (state %i)\n", cur_state);
			device.card_rca = 0;
			return -1;
		}
	}
    return 0;
}

static int sd_do_data_command(int is_write, uint8_t *buf, size_t buf_size, uint32_t block_no){
	// PLSS table 4.20 - SDSC cards use byte addresses rather than block addresses
	if(!device.card_supports_sdhc)
		block_no *= 512;

	// This is as per HCSS 3.7.2.1
	if(buf_size < device.block_size)
	{
	    printf("SD: do_data_command() called with buffer size (%i) less than "
            "block size (%i)\n", buf_size, device.block_size);
        return -1;
	}

	device.blocks_to_transfer = buf_size / device.block_size;
	if(buf_size % device.block_size)
	{
	    printf("SD: do_data_command() called with buffer size (%i) not an "
            "exact multiple of block size (%i)\n", buf_size, device.block_size);
        return -1;
	}
	device.buf = buf;

	// Decide on the command to use
	int command;
	if(is_write)
	{
	    if(device.blocks_to_transfer > 1)
            command = WRITE_MULTIPLE_BLOCK;
        else
            command = WRITE_BLOCK;
	}
	else
    {
        if(device.blocks_to_transfer > 1)
            command = READ_MULTIPLE_BLOCK;
        else
            command = READ_SINGLE_BLOCK;
    }

	int retry_count = 0;
	int max_retries = 3;
    while(retry_count < max_retries){
        device.use_sdma = 0;
        sd_issue_command(command, block_no, 5000000);
        if(SUCCESS(device))
            break;
        else
        {
            printf("SD: error sending CMD%i, ", command);
            printf("error = %08x.  ", device.last_error);
            retry_count++;
            if(retry_count < max_retries)
                printf("Retrying...\n");
            else
                printf("Giving up.\n");
        }
    }
	if(retry_count == max_retries)
    {
        device.card_rca = 0;
        return -1;
    }

    return 0;
}

int sd_read_write(int is_write, uint8_t *buf, size_t buf_size, uint32_t block_no){
	// Check the status of the card
    if(sd_ensure_data_mode() != 0)
        return -1;
    
    if(sd_do_data_command(is_write, buf, buf_size, block_no) < 0)
        return -1;
    
#ifdef EMMC_DEBUG
    if(is_write)
        printf("SD: write read successful\n");
    else
	    printf("SD: data read successful\n");
#endif

    return buf_size;
}

static void sd_host_interrupt_handler(void){
    printf("SD HANDLR\tresp0:%x\tresp1:%x\tresp2:%x\tresp3:%x\t",
    host_regs->resp[0], host_regs->resp[1], host_regs->resp[2], host_regs->resp[3]);

}

static void sd_host_interrupt_clearer(void){
    printf("SD CLEAR\t");
    // Store and clear the flags
    interrupt_status = host_regs->interrupt_flags;
    host_regs->interrupt_flags.val = 0;
}

/*void sd_init(void){
    DISABLE_INTERRUPTS();

    if(!ON_EMU)
        bcm2835_setpower(POWER_SD, 1);

    register_irq_handler(SD_HOST_CONTROLLER,
    sd_host_interrupt_handler, sd_host_interrupt_clearer);

    union reg_control0 control0 = host_regs->control0;
    control0.hctl_dwidth = 1;
    host_regs->control0 = control0;

    union reg_control1 control1 = host_regs->control1;
    control1.clk_en = 1;
    host_regs->control1 = control1;

    // Set data buff to the register.
    void* data = data_buff;
    host_regs->data = (uint32_t) data;

    // Set the flags for every category.
    union reg_interrupt interrupt_flags_mask = host_regs->interrupt_flags_mask;
    interrupt_flags_mask.val = 0x17f7137;
    host_regs->interrupt_flags_mask = interrupt_flags_mask;

    // Enable the interrupts for every category.
    union reg_interrupt interrupt_enable = host_regs->interrupt_enable;
    interrupt_enable.val = 0x17f7137;
    host_regs->interrupt_enable = interrupt_enable;

    ENABLE_INTERRUPTS();
}*/

void sd_read_cid_or_csd(uint32_t cmd_index){
    union reg_blk_size_cnt blk_size_cnt = host_regs->blk_size_cnt;
    union reg_cmdtm cmdtm = host_regs->cmdtm;
    

    blk_size_cnt.blk_cnt = 1;
    blk_size_cnt.blk_size = 1023;

    // Send stop transmission cmd after transfer.
    cmdtm.tm_auto_cmd_en = 1;

    // Data direction card to host.
    cmdtm.tm_dat_dir = 1;

    // 136 bit response
    cmdtm.cmd_rspns_type = 1;

    // Data transfer command.
    // cmdtm.cmd_isdata = 1;

    // Set the command index.
    cmdtm.cmd_index = cmd_index;
    
    host_regs->blk_size_cnt = blk_size_cnt;
    host_regs->cmdtm = cmdtm;
}

void sd_log_regs(void){
    // TODO log control register etc.
    printf("control0:%x\tcontrol1:%x\tcontrol2:%x\tstatus:%x\t",
    host_regs->control0, host_regs->control1, host_regs->control2,
    host_regs->status);
}