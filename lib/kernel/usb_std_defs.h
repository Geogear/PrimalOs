#ifndef _USB_STD_DEFS_H_
#define _USB_STD_DEFS_H_

#include <kernel/kernel.h>

/** Directions of USB transfers (defined relative to the host) */
enum usb_direction {
    USB_DIRECTION_OUT = 0,  /* Host to device */
    USB_DIRECTION_IN = 1    /* Device to host */
};

enum usb_speed {
    USB_SPEED_HIGH = 0,
    USB_SPEED_FULL = 1,
    USB_SPEED_LOW  = 2,
};

enum dwc_usb_pid {
    DWC_USB_PID_DATA0 = 0,
    DWC_USB_PID_DATA1 = 2,
    DWC_USB_PID_DATA2 = 1,
    DWC_USB_PID_SETUP = 3,
};

enum usb_transfer_type {
    USB_TRANSFER_TYPE_CONTROL     = 0,
    USB_TRANSFER_TYPE_ISOCHRONOUS = 1,
    USB_TRANSFER_TYPE_BULK        = 2,
    USB_TRANSFER_TYPE_INTERRUPT   = 3,
};

/** Standard USB descriptor types.  See Table 9-5 in Section 9.4 of the USB 2.0
 * specification.  */
enum usb_descriptor_type {
    USB_DESCRIPTOR_TYPE_DEVICE        = 1,
    USB_DESCRIPTOR_TYPE_CONFIGURATION = 2,
    USB_DESCRIPTOR_TYPE_STRING        = 3,
    USB_DESCRIPTOR_TYPE_INTERFACE     = 4,
    USB_DESCRIPTOR_TYPE_ENDPOINT      = 5,
    USB_DESCRIPTOR_TYPE_HUB           = 0x29,
};

/** USB request types (bits 6..5 of bmRequestType).  See Table 9-2 of Section
 * 9.3 of the USB 2.0 specification.  */
enum usb_request_type {
    USB_REQUEST_TYPE_STANDARD = 0,
    USB_REQUEST_TYPE_CLASS    = 1,
    USB_REQUEST_TYPE_VENDOR   = 2,
    USB_REQUEST_TYPE_RESERVED = 3,
};

/** USB request recipients (bits 4..0 of bmRequestType).  See Table 9-2 of
 * Section 9.3 of the USB 2.0 specification.  */
enum usb_request_recipient {
    USB_REQUEST_RECIPIENT_DEVICE    = 0,
    USB_REQUEST_RECIPIENT_INTERFACE = 1,
    USB_REQUEST_RECIPIENT_ENDPOINT  = 2,
    USB_REQUEST_RECIPIENT_OTHER     = 3,
};

/** Values of the bitfields within the bmRequestType member of the setup data.
 * See Table 9-2 of Section 9.3 of the USB 2.0 specification.  */
enum usb_bmRequestType_fields {
    USB_BMREQUESTTYPE_DIR_OUT             = (USB_DIRECTION_OUT << 7),
    USB_BMREQUESTTYPE_DIR_IN              = (USB_DIRECTION_IN << 7),
    USB_BMREQUESTTYPE_DIR_MASK            = (0x1 << 7),
    USB_BMREQUESTTYPE_TYPE_STANDARD       = (USB_REQUEST_TYPE_STANDARD << 5),
    USB_BMREQUESTTYPE_TYPE_CLASS          = (USB_REQUEST_TYPE_CLASS << 5),
    USB_BMREQUESTTYPE_TYPE_VENDOR         = (USB_REQUEST_TYPE_VENDOR << 5),
    USB_BMREQUESTTYPE_TYPE_RESERVED       = (USB_REQUEST_TYPE_RESERVED << 5),
    USB_BMREQUESTTYPE_TYPE_MASK           = (0x3 << 5),
    USB_BMREQUESTTYPE_RECIPIENT_DEVICE    = (USB_REQUEST_RECIPIENT_DEVICE << 0),
    USB_BMREQUESTTYPE_RECIPIENT_INTERFACE = (USB_REQUEST_RECIPIENT_INTERFACE << 0),
    USB_BMREQUESTTYPE_RECIPIENT_ENDPOINT  = (USB_REQUEST_RECIPIENT_ENDPOINT << 0),
    USB_BMREQUESTTYPE_RECIPIENT_OTHER     = (USB_REQUEST_RECIPIENT_OTHER << 0),
    USB_BMREQUESTTYPE_RECIPIENT_MASK      = (0x1f << 0),
};

/** Standard USB device requests.  See Table 9-3 in Section 9.4 of the USB 2.0
 * specification.  */
enum usb_device_request {
    USB_DEVICE_REQUEST_GET_STATUS        = 0,
    USB_DEVICE_REQUEST_CLEAR_FEATURE     = 1,
    USB_DEVICE_REQUEST_SET_FEATURE       = 3,
    USB_DEVICE_REQUEST_SET_ADDRESS       = 5,
    USB_DEVICE_REQUEST_GET_DESCRIPTOR    = 6,
    USB_DEVICE_REQUEST_SET_DESCRIPTOR    = 7,
    USB_DEVICE_REQUEST_GET_CONFIGURATION = 8,
    USB_DEVICE_REQUEST_SET_CONFIGURATION = 9,
    USB_DEVICE_REQUEST_GET_INTERFACE     = 10,
    USB_DEVICE_REQUEST_SET_INTERFACE     = 11,
    USB_DEVICE_REQUEST_SYNCH_FRAME       = 12,
};

/** Some of the commonly used USB class codes.  Note that only the hub class is
 * defined in the USB 2.0 specification itself; the other standard class codes
 * are defined in additional specifications.  */
enum usb_class_code {
    USB_CLASS_CODE_INTERFACE_SPECIFIC             = 0x00,
    USB_CLASS_CODE_AUDIO                          = 0x01,
    USB_CLASS_CODE_COMMUNICATIONS_AND_CDC_CONTROL = 0x02,
    USB_CLASS_CODE_HID                            = 0x03,
    USB_CLASS_CODE_IMAGE                          = 0x06,
    USB_CLASS_CODE_PRINTER                        = 0x07,
    USB_CLASS_CODE_MASS_STORAGE                   = 0x08,
    USB_CLASS_CODE_HUB                            = 0x09,
    USB_CLASS_CODE_VIDEO                          = 0x0e,
    USB_CLASS_CODE_WIRELESS_CONTROLLER            = 0xe0,
    USB_CLASS_CODE_MISCELLANEOUS                  = 0xef,
    USB_CLASS_CODE_VENDOR_SPECIFIC                = 0xff,
};

/** Standard SETUP data for a USB control request.  See Table 9-2 in Section 9.3
 * of the USB 2.0 specification.  */
struct usb_control_setup_data {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __packed;

/** Fields that begin every standard USB descriptor.  */
struct usb_descriptor_header {
    uint8_t bLength;
    uint8_t bDescriptorType;
} __packed;

/** Standard format of USB device descriptors.  See Table 9-8 in 9.6.1 of the
 * USB 2.0 specification.  */
struct usb_device_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __packed;

/** Standard format of USB configuration descriptors.  See Table 9-10 in Section
 * 9.6.3 of the USB 2.0 specification.  */
struct usb_configuration_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
} __packed;

/** Standard format of USB interface descriptors.  See Table 9-12 in Section
 * 9.6.6 of the USB 2.0 specification.  */
struct usb_interface_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __packed;

/** Standard format of USB endpoint descriptors.  See Table 9-13 in Section
 * 9.6.6 of the USB 2.0 specification.  */
struct usb_endpoint_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} __packed;

struct hid_descriptor{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdHID;
    uint8_t bCountryCode;
    uint8_t bNumDescriptors;
    uint8_t bDescriptorTypeR;
    uint16_t wDescriptorLength;
} __packed;

#define KEYBOARD_DEVICE_ADDRESS 1

#endif