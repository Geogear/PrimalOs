#ifndef USB_H
#define USB_H

#include <stdint.h>
#include <kernel/peripheral.h>

#define USB_BASE (USB_OFFSET + PERIPHERAL_BASE)
#define USB_MDIO_CONTL_OFFSET 0x80
#define USB_MDIO_DATA_OFFSET 0x84
#define USB_VBUS_OFFSET 0x88
#define USB_GAHBCFG_OFFSET 0x8

typedef struct{
    uint32_t mdi:16; //[15:0]
    uint32_t mdc_ratio:4; //[19:16]
    uint32_t freerun:1; //[20]
    uint32_t bb_enbl:1; //[21]
    uint32_t bb_mdc:1; //[22] Direct write (bitbash) MDC output
    uint32_t bb_mdo:1; //[23] Direct write (bitbash) MDO output
    uint32_t unused:7; //[30:24]
    uint32_t mdio_busy:1; //[31]
}usb_mdio_control_t;

typedef struct
{
    uint32_t mdio_data;
}usb_mdio_data_t;


typedef struct{
    uint32_t utmisrp_sessend:1; //[0]
    uint32_t utmiotg_vbusvalid:1; //[1]
    uint32_t utmiotg_bvalid:1; //[2]
    uint32_t utmiotg_avalid:1; //[3]
    uint32_t utmiotg_drvvbus:1; //[4]
    uint32_t utmisrp_chrgvbus:1; //[5]
    uint32_t utmisrp_dischrgvbus:1; //[6]
    uint32_t afe_non_driving:1; //[7]
    uint32_t vbus_irq_en:1; // [8]
    uint32_t vbus_irq:1; //[9]
    uint32_t unused1:6; //[15:10]
    uint32_t axi_priority:4; //[19:16]
    uint32_t unused2:12; //[31:20]
}usb_vbus_t;

typedef struct{
    //TODO
}usb_ahb_confg_t;

void usb_init(void);

#endif