#ifndef SDHOSTREGS_H
#define SDHOSTREGS_H

#include <stdint.h>
#include <stddef.h>
#include <kernel/peripheral.h>
#include <kernel/sd_host_defs.h>

#define SD_REGS_BASE (SDHCI_REGS_OFFSET + PERIPHERAL_BASE)

struct emmc_regs{
    // 0x0 ACMD23 Argument
    uint32_t arg2;
    
    // 0x4 Block Size and Count
    union reg_blk_size_cnt{
        uint32_t val;
        struct{
            /* Block size in bytes. */
            uint32_t blk_size : 10;

            uint32_t reserved : 6;

            /* Number of blocks to be transferred. */
            uint32_t blk_cnt : 16;
        };
    }blk_size_cnt;

    // 0x8 ARG1 Register
    uint32_t arg1;

    // 0xC Command and Transfer Mode
    union reg_cmdtm{
        uint32_t val;
        struct{
            uint32_t res : 1;
            
            /**
             * Enable the block counter for multiple block transfers
             * 0 = disabled, 1 = enabled
             */
            uint32_t tm_blkcnt_en : 1;

            /**
             * Select the command to be send after completion
             * of a data transfer.
             * 00 = no command, 01 = CMD12
             * 10 = CMD23, 11 = reserved
             */
            uint32_t tm_auto_cmd_en : 2;

            /**
             * Direction of data transfer
             * 0 = host to card, 1 = card to host
             */
            uint32_t tm_dat_dir : 1;

            /**
             * Tpe of data transfer
             * 0 = single block, 1 = multi block
             */
            uint32_t tm_multi_block : 1;

            uint32_t res2 : 10;

            /**
             * Type of expected response from card
             * 00 = no response, 01 = 136 bits response
             * 10 = 48 bits response, 11 = 48 response using busy
             */
            uint32_t cmd_rspns_type : 2;

            uint32_t res3 : 1;

            /**
             * check the responses crc
             * 0 = disabled, 1 = enabled
             */
            uint32_t cmd_crcchk_en : 1;

            /**
             * Check that response has same index as command
             * 0 = disabled
             * 1 = enabled
             */
            uint32_t cmd_ixchk_en : 1;

            /**
             * Command involves data transfer
             * 0 = no dt command, 1 = dt command
             */
            uint32_t cmd_isdata : 1;

            /**
             * Type of command to be issued to the card
             * 00 = normal, 01 = suspend (the current dt)
             * 10 = resume (the last dt), 11 = abort (the current dt)
             */
            uint32_t cmd_type : 2;

            /* Index of the command to be issued to the card. */
            uint32_t cmd_index : 6;
        };
    }cmdtm;

    // 0x10 - 0x14 - 0x18- 0x1C RESP bits 127:0
    uint32_t resp[4];

    // 0x20 DATA
    /* Bit 1 of the INTERRUPT register can be used to check if data is available. */
    uint32_t data;

    // 0x24 STATUS
    /**
     * Intended for debugging, hardware sets this,
     * so not recommended to use with polling, instead
     * use INTERRUPT register which implements a handshake mechanism
     * which makes it impossible to miss a change when polling
     */
    union reg_status{
        uint32_t val;
        struct{
            /* Command line still used by previous command. */
            uint32_t cmd_inhibit : 1;

            /* Data lines still used by previous data transfer. */
            uint32_t dat_inhibit : 1;

            /* At least one data line is active. */
            uint32_t dat_active : 1;

            uint32_t res0 : 5;

            /* New data can be written to EMMC. */
            uint32_t write_transfer : 1;

            /* New data can be read from EMMC. */
            uint32_t read_transfer : 1;

            uint32_t res1 : 10;

            /* Value of data lines DAT3 to DAT0. */
            uint32_t dat_level0 : 4;

            /* Value of command line cmd. */
            uint32_t cmd_level : 1;

            /* Value of data lines DAT7 to DAT4. */
            uint32_t data_level1 : 4;

            uint32_t res2 : 3;
        };
    }status;

    // 0x28 Host Configuration bits CONTROL0
    union reg_control0{
        uint32_t val;
        struct{
            uint32_t res0 : 1;

            /* Use 4 data lines. */
            uint32_t hctl_dwidth : 1;

            /**
             * Select high speed mode (i.e. DAT and CMD
             * lines change on the rising CLK edge)
             */
            uint32_t hctl_hs_en : 1;

            uint32_t res1 : 2;

            /* Use 8 data lines. */
            uint32_t hctl_8bit : 1;

            uint32_t res2: 10;

            /**
             * Stop the current transaction at the next block gap.
             * 0 = ignore, 1 = stop.
             */
            uint32_t gap_stop : 1;

            /**
             * Restart a transaction which was stopped using the GAP_STOP bit.
             * 0 = ignore, 1 = restart.
             */
            uint32_t gap_restart : 1;

            /* Use DAT2 read-wait protocol for SDIO cards supporting this. */
            uint32_t readwait_en : 1;

            /* Enable SDIO interrupt at block gap (only valid ifthe HCTL_DWIDTH bit is set. */
            uint32_t gap_ien : 1;

            /**
             * SPI mode enable.
             * 0 = normal, 1 = SPI mode
             */
            uint32_t spi_mode : 1;

            /**
             * Boot mode access
             * 0 = stop boot mode access, 1 = start boot mode access.
             */
            uint32_t boot_en : 1;

            /* Enable alternate boot mode access. */
            uint32_t alt_boot_en : 1;
        };
    }control0;

    // 0x2C Host Configuration bits CONTROL1
    union reg_control1{
        uint32_t val;
        struct{
            /* Clock enable for internal EMMC clocks for power saving. */
            uint32_t clk_intlen : 1;

            /* SD clock stable. */
            uint32_t clk_stable : 1;

            /* SD clock enable. */
            uint32_t clk_en : 1;

            uint32_t res0 : 2;

            /**
             * Mode of clock generation
             * 0 = divided, 1 = programmable
             */
            uint32_t clk_gensel : 1;

            /* SD clock base divider MSBs. */
            uint32_t clk_freq_ms2 : 2;

            /* SD clock base divider LSBs. */
            uint32_t clk_freq8 : 8;

            /**
             * Data timeout unit exponent.
             * 1111 = disabled, x = TMCLK * 2^(x+13)
             */
            uint32_t data_tounit : 4;

            uint32_t res1 : 4;

            /* Reset the complete host circuit. */
            uint32_t srst_hc : 1;

            /* Reset the command handling circuit. */
            uint32_t srst_cmd : 1;

            /* Reset the data handling circuit. */
            uint32_t srst_data : 1;

            uint32_t res2 : 5;
        };
    }control1;

    // 0x30 Interrupt Flags INTERRUPT
    union reg_interrupt{
        uint32_t val;
        struct{
            /* Command has finished. */
            uint32_t cmd_done : 1;

            /* Data transfer has finished. */
            uint32_t data_done : 1;

            /* Data transfer has stopeed at block gap. */
            uint32_t block_gap : 1;

            uint32_t res0 : 1;

            /* Data can be written to DATA register. */
            uint32_t write_rdy : 1;

            /* Data register contains data to be read. */
            uint32_t read_rdy : 1;

            uint32_t res1 : 2;

            /* Card made interrupt request. */
            uint32_t card : 1;

            uint32_t res2 : 3;

            /* Clock retune request was made. */
            uint32_t retune : 1;

            /* Boot acknowledge has been received. */
            uint32_t bootack : 1;

            /* Boot operation has terminated. */
            uint32_t endboot : 1;

            /* An error has occured. */
            uint32_t err : 1;

            /* Timeout on command line. */
            uint32_t cto_err : 1;

            /* Command crc error. */
            uint32_t ccrc_err : 1;

            /* End bit on command line not 1*/
            uint32_t cend_err : 1;

            /* Incorrect command index in response. */
            uint32_t cbad_err : 1;

            /* Timeout on data line. */
            uint32_t dto_err : 1;

            /* Data crc error. */
            uint32_t dcrc_err : 1;

            /* End bit on data line not 1. */
            uint32_t dend_err : 1;

            uint32_t res3 : 1;

            /* Auto command error. */
            uint32_t acmd_err : 1;

            uint32_t res : 7;
        };
    }interrupt_flags;

    // 0x34 Interrupt Flag Enable IRPT_MASK
    union reg_interrupt interrupt_flags_mask;

    // 0x38 Interrupt Generation Enable IRPT_EN
    union reg_interrupt interrupt_enable;

    // 0x3C Host Configuration bits CONTROL2
    union reg_control2{
        uint32_t val;
        struct{
            /* Auto command not executed due to an error. */
            uint32_t acnox_err : 1;

            /* Timeout occured during auto command execution. */
            uint32_t acto_err : 1;

            /* Command CRC error occurred during auto command execution. */
            uint32_t accrc_err : 1;

            /* End bit is not 1 during auto command execution. */
            uint32_t acend_err : 1;

            /* Command index error occured during auto commands execution. */
            uint32_t acbad_err : 1;

            uint32_t res0 : 2;

            /* Error occured during auto command CMD12 execution.  */
            uint32_t notc12_err : 1;

            uint32_t res1 : 8;

            /**
             * Select the speed mode of the SD card.
             * 000 = SDR12, 001 = SDR25, 010 = SDR50
             * 011 = SDR104, 100 = DDR50, other = reserved.
             */
            uint32_t uhsmode : 3;

            uint32_t res2 : 3;

            /** 
             * Start tuning the SD clock
             * 0 = not tuned or tuning complete, 1 = tuning
             */
            uint32_t tuneon : 1;

            /* Tuned clock is uesed for sampling data. */
            uint32_t tuned : 1;

            uint32_t res3 : 8;
        };
    }control2;

    // 0x40 - 0x44 - 0x48 - 0x4C
    uint32_t not_def0[4];

    // 0x50 Force Interrupt Event FORCE_IRPT
    // Used to fake interrupts for debugging.
    union reg_interrupt interrupt_force;

    // 0x54 - 0x58 - 0x5C - 0x60 - 0x64 - 0x68 - 0x6C
    uint32_t not_def1[7];

    // 0x70 Timeout in boot mode BOOT_TIMEOUT
    uint32_t boot_timeout;

    // 0x74 Debug Bus Configuration DBG_SEL
    uint32_t dbg_sel;

    // 0x78 - 0x7C
    uint32_t not_def2[2];

    // 0x80 Extension FIFO Configuration EXRDFIFO_CFG
    uint32_t exrdfifo_cfg;

    // 0x84 Extension FIFO Enable EXRDFIFO_EN
    uint32_t exrdfifo_en;

    // 0x88 Delay per card clock tuning step TUNE_STEP
    uint32_t tune_step;

    // 0x8C Card clock tuning steps for SDR TUNE_STEPS_STD
    uint32_t tune_steps_std;

    // 0x90 Card clock tuning steps for DDR TUNE_STEPS_DDR
    uint32_t tune_steps_ddr;

    /**
     *  0x94 - 0x98 - 0x9C - 0xA0
     *  0xA4 - 0xA8 - 0xAC - 0xB0
     *  0xB4 - 0xB8 - 0xBC - 0xC0
     *  0xC4 - 0xC8 - 0xCC - 0xD0
     *  0xD4 - 0xD8 - 0xDC - 0xE0
     *  0xE4 - 0xE8 - 0xEC
     */
    uint32_t not_def3[23];

    // 0xF0 SPI Interrupt Support SPI_INT_SPT
    uint32_t spi_int_spt;

    // 0xF4 - 0xF8
    uint32_t not_def4[2];

    // 0xFC Slot Interrupt Status and Version SLOTISR_VER
    uint32_t slotisr_ver;
};

int sd_card_init(void);
int sd_read_write(int is_write, uint8_t *buf, size_t buf_size, uint32_t block_no);

void sd_read_cid_or_csd(uint32_t cmd_index);
void sd_log_regs(void);

#endif