/*
 * usb_config.h - Settings for Microchip's MLA USB device stack (mla_usb/).
 *
 * The stack includes this file by name, with no folder, so it must sit in a
 * folder on the motherboard's include path. Every value the stack reads from the
 * application is set here; the stack's own files are left unchanged.
 */
#ifndef USB_CONFIG_H
#define USB_CONFIG_H

#include "common/protocol.h"

/* ------------------------------------------------------------------------ */
/* Stack                                                                    */
/* ------------------------------------------------------------------------ */
#define USB_EP0_BUFF_SIZE           8u      /* control endpoint packet size  */
#define USB_MAX_NUM_INT             1u      /* interfaces in the configuration */
#define USB_MAX_EP_NUMBER           1u      /* highest endpoint used: EP1    */

#define USB_USER_DEVICE_DESCRIPTOR          &device_dsc
#define USB_USER_DEVICE_DESCRIPTOR_INCLUDE  extern const USB_DEVICE_DESCRIPTOR device_dsc
#define USB_USER_CONFIG_DESCRIPTOR          USB_CD_Ptr
#define USB_USER_CONFIG_DESCRIPTOR_INCLUDE  extern const uint8_t *const USB_CD_Ptr[]

#define USB_NUM_STRING_DESCRIPTORS  3u      /* language, manufacturer, product */

/* Ping-pong buffers on every endpoint, so the hardware can take the next packet
 * while the firmware is still handling the last one. */
#define USB_PING_PONG_MODE          USB_PING_PONG__FULL_PING_PONG

/* The stack runs from the high-priority interrupt. The main loop can block for
 * tens of milliseconds (EEPROM writes, a chain enumeration), which is too long
 * for the host's control requests to wait, so polling mode is not an option. */
#define USB_INTERRUPT

#define USB_PULLUP_OPTION           USB_PULLUP_ENABLE
#define USB_TRANSCEIVER_OPTION      USB_INTERNAL_TRANSCEIVER
#define USB_SPEED_OPTION            USB_FULL_SPEED

/* Lets a control transfer finish even if the firmware is slow to arm its
 * status stage, as the USB 2.0 timing rules require. */
#define USB_ENABLE_STATUS_STAGE_TIMEOUTS
#define USB_STATUS_STAGE_TIMEOUT    ((uint8_t)45)   /* ms */

#define USB_SUPPORT_DEVICE

/* The stack calls this for every USB event. Defined in usb_app.c. */
#define USER_USB_CALLBACK_EVENT_HANDLER     usb_app_event_handler

/* ------------------------------------------------------------------------ */
/* HID class                                                                */
/* ------------------------------------------------------------------------ */
#define USB_USE_HID

#define HID_INTF_ID                 0x00u
#define HID_EP                      1u
#define HID_INT_IN_EP_SIZE          USB_REPORT_LEN
#define HID_INT_OUT_EP_SIZE         USB_REPORT_LEN
#define HID_NUM_OF_DSC              1u
#define HID_RPT01_SIZE              29u     /* bytes in hid_rpt01, usb_descriptors.c */

#endif /* USB_CONFIG_H */
