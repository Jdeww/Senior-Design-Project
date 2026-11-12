/*
 * usb_descriptors.c - What the motherboard tells the PC about itself during USB
 * enumeration. The MLA stack sends these; the names device_dsc, configDescriptor1,
 * USB_CD_Ptr, USB_SD_Ptr and hid_rpt01 are the ones it looks for.
 *
 * The device is a vendor-defined HID (usage page 0xFF00), so the PC uses its
 * built-in HID driver and the PC app reads the raw 8-byte reports by VID/PID.
 * One interface, with an interrupt IN endpoint for reports to the PC and an
 * interrupt OUT endpoint for its commands, both on EP1 and polled every 1 ms.
 */
#include "usb_config.h"
#include "usb.h"
#include "usb_device_hid.h"

/* PLACEHOLDER IDs: 0x04D8 is Microchip's vendor ID, and 0x003F is the product ID
 * of its MLA custom HID demo. They are fine for bench testing, but the finished
 * device needs its own pair. Microchip sublicenses product IDs under 0x04D8 for
 * free to projects using its USB chips. Change the PC app to match. */
#define USB_VID                 0x04D8u
#define USB_PID                 0x003Fu
#define USB_DEVICE_RELEASE      0x0100u     /* bcdDevice, version 1.00 */

#define USB_MAX_POWER_MA        100u        /* PLACEHOLDER: whole chain, measured */

#define CONFIG_DESC_TOTAL_LEN   41u         /* 9 + 9 + 9 + 7 + 7 */

/* ------------------------------------------------------------------------ */
/* Device descriptor                                                        */
/* ------------------------------------------------------------------------ */
const USB_DEVICE_DESCRIPTOR device_dsc = {
    0x12,                       /* bLength                                  */
    USB_DESCRIPTOR_DEVICE,
    0x0200,                     /* USB 2.0                                  */
    0x00,                       /* class is set by the interface            */
    0x00,
    0x00,
    USB_EP0_BUFF_SIZE,
    USB_VID,
    USB_PID,
    USB_DEVICE_RELEASE,
    0x01,                       /* manufacturer string                      */
    0x02,                       /* product string                           */
    0x00,                       /* no serial number string                  */
    0x01                        /* one configuration                        */
};

/* ------------------------------------------------------------------------ */
/* Configuration descriptor                                                 */
/* ------------------------------------------------------------------------ */
/* usb_device_hid.c finds the HID descriptor by its fixed offset of 18 bytes,
 * so the configuration and interface descriptors must stay first. */
const uint8_t configDescriptor1[CONFIG_DESC_TOTAL_LEN] = {
    /* Configuration */
    0x09,
    USB_DESCRIPTOR_CONFIGURATION,
    CONFIG_DESC_TOTAL_LEN, 0x00,
    1,                          /* interfaces                               */
    1,                          /* this configuration's number              */
    0,                          /* no string                                */
    _DEFAULT,                   /* bus powered, no remote wakeup            */
    USB_MAX_POWER_MA / 2u,      /* in 2 mA units                            */

    /* Interface */
    0x09,
    USB_DESCRIPTOR_INTERFACE,
    HID_INTF_ID,
    0,                          /* alternate setting                        */
    2,                          /* endpoints: IN and OUT                    */
    0x03,                       /* HID class                                */
    0x00,                       /* no boot subclass                         */
    0x00,                       /* no boot protocol                         */
    0,                          /* no string                                */

    /* HID class descriptor (offset 18) */
    0x09,
    DSC_HID,
    0x11, 0x01,                 /* HID 1.11                                 */
    0x00,                       /* no country code                          */
    HID_NUM_OF_DSC,
    DSC_RPT,
    HID_RPT01_SIZE, 0x00,

    /* Endpoint 1 IN: reports to the PC */
    0x07,
    USB_DESCRIPTOR_ENDPOINT,
    _EP01_IN,
    _INTERRUPT,
    HID_INT_IN_EP_SIZE, 0x00,
    0x01,                       /* polled every 1 ms                        */

    /* Endpoint 1 OUT: commands from the PC */
    0x07,
    USB_DESCRIPTOR_ENDPOINT,
    _EP01_OUT,
    _INTERRUPT,
    HID_INT_OUT_EP_SIZE, 0x00,
    0x01
};

const uint8_t *const USB_CD_Ptr[] = {
    (const uint8_t *const)&configDescriptor1
};

/* ------------------------------------------------------------------------ */
/* HID report descriptor                                                    */
/* ------------------------------------------------------------------------ */
/* Eight bytes of vendor data in each direction, with no report ID. The bytes'
 * meaning is the report format in protocol.h, which the HID layer never sees. */
const struct {
    uint8_t report[HID_RPT01_SIZE];
} hid_rpt01 = {{
    0x06, 0x00, 0xFF,           /* Usage Page (vendor-defined 0xFF00)       */
    0x09, 0x01,                 /* Usage (vendor 1)                         */
    0xA1, 0x01,                 /* Collection (Application)                 */
    0x19, 0x01,                 /*   Usage Minimum (1)                      */
    0x29, USB_REPORT_LEN,       /*   Usage Maximum (8)                      */
    0x15, 0x00,                 /*   Logical Minimum (0)                    */
    0x26, 0xFF, 0x00,           /*   Logical Maximum (255)                  */
    0x75, 0x08,                 /*   Report Size (8 bits)                   */
    0x95, USB_REPORT_LEN,       /*   Report Count (8)                       */
    0x81, 0x02,                 /*   Input (Data, Variable, Absolute)       */
    0x19, 0x01,                 /*   Usage Minimum (1)                      */
    0x29, USB_REPORT_LEN,       /*   Usage Maximum (8)                      */
    0x91, 0x02,                 /*   Output (Data, Variable, Absolute)      */
    0xC0                        /* End Collection                           */
}};

/* ------------------------------------------------------------------------ */
/* String descriptors (UTF-16)                                              */
/* ------------------------------------------------------------------------ */
static const struct {
    uint8_t  bLength;
    uint8_t  bDscType;
    uint16_t string[1];
} sd000 = { sizeof(sd000), USB_DESCRIPTOR_STRING, { 0x0409 } };   /* English (US) */

/* PLACEHOLDER: replace with the team or organisation name. */
static const struct {
    uint8_t  bLength;
    uint8_t  bDscType;
    uint16_t string[14];
} sd001 = { sizeof(sd001), USB_DESCRIPTOR_STRING,
    { 'M','a','c','r','o',' ','P','a','d',' ','T','e','a','m' } };

static const struct {
    uint8_t  bLength;
    uint8_t  bDscType;
    uint16_t string[17];
} sd002 = { sizeof(sd002), USB_DESCRIPTOR_STRING,
    { 'M','o','d','u','l','a','r',' ','M','a','c','r','o',' ','P','a','d' } };

const uint8_t *const USB_SD_Ptr[USB_NUM_STRING_DESCRIPTORS] = {
    (const uint8_t *const)&sd000,
    (const uint8_t *const)&sd001,
    (const uint8_t *const)&sd002
};
