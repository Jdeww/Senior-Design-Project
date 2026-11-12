/*
 * usb_app.c - See usb_app.h.
 *
 * The stack's callback runs in the high-priority interrupt; everything else here
 * runs in the main loop. The main loop masks the USB interrupt while it hands a
 * buffer to the stack, so the two never change the endpoint state at once.
 */
#include "motherboard/board.h"
#include "motherboard/usb_app.h"
#include "motherboard/module_manager.h"
#include "motherboard/report_queue.h"
#include "common/protocol.h"
#include "usb_config.h"
#include "usb.h"
#include "usb_device_hid.h"

/* The USB module can only reach USB RAM (banks 4 to 7), so the endpoint buffers
 * sit at fixed addresses straight after the stack's own endpoint 0 buffers. */
#define IN_BUFFER_ADDRESS   (CTRL_TRF_DATA_ADDRESS + USB_EP0_BUFF_SIZE)
#define OUT_BUFFER_ADDRESS  (IN_BUFFER_ADDRESS + HID_INT_IN_EP_SIZE)

#if (OUT_BUFFER_ADDRESS + HID_INT_OUT_EP_SIZE) > 0x800
    #error "usb_app: endpoint buffers do not fit in USB RAM"
#endif

static volatile uint8_t s_in_buffer[HID_INT_IN_EP_SIZE] __at(IN_BUFFER_ADDRESS);
static volatile uint8_t s_out_buffer[HID_INT_OUT_EP_SIZE] __at(OUT_BUFFER_ADDRESS);

/* Busy while the hardware owns the buffer. Set by the callback and the main loop. */
static volatile USB_HANDLE s_in_handle;
static volatile USB_HANDLE s_out_handle;

void usb_app_init(void)
{
    s_in_handle = 0;
    s_out_handle = 0;
    USBDeviceInit();
    USBDeviceAttach();
}

bool usb_app_ready(void)
{
    return (USBGetDeviceState() == CONFIGURED_STATE) && !USBIsDeviceSuspended();
}

static void arm_out(void)
{
    USBMaskInterrupts();
    s_out_handle = HIDRxPacket(HID_EP, (uint8_t *)s_out_buffer, HID_INT_OUT_EP_SIZE);
    USBUnmaskInterrupts();
}

static void send_in(void)
{
    USBMaskInterrupts();
    s_in_handle = HIDTxPacket(HID_EP, (uint8_t *)s_in_buffer, HID_INT_IN_EP_SIZE);
    USBUnmaskInterrupts();
}

/* Returns false if the command can't be carried out yet. The OUT buffer is then
 * held, so the PC's next command waits (the hardware NAKs it) instead of being
 * lost. */
static bool run_command(uint8_t command)
{
    switch (command) {
        case CMD_LIST_MODULES:
            module_manager_list_modules();
            return true;
        case CMD_FW_VERSION:
            return report_push_fw_version(FW_VERSION);
        default:
            return true;    /* unknown commands are ignored */
    }
}

void usb_app_task(void)
{
    if (!usb_app_ready()) {
        return;
    }

    /* Not busy means the PC has sent a command into the buffer. */
    if (!HIDRxHandleBusy(s_out_handle)) {
        if (run_command(s_out_buffer[0])) {
            arm_out();
        }
    }

    /* One report per frame: the PC polls EP1 IN once every 1 ms. */
    if (!HIDTxHandleBusy(s_in_handle)) {
        if (report_queue_pop((uint8_t *)s_in_buffer)) {
            send_in();
        }
    }
}

/* Runs in the high-priority interrupt, from USBDeviceTasks(). */
bool USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, uint16_t size)
{
    (void)pdata;
    (void)size;

    switch ((int)event) {
        case EVENT_CONFIGURED:
            /* The PC has picked the configuration: open EP1 and wait for a
             * command. Anything that was in flight before is gone. */
            USBEnableEndpoint(HID_EP, USB_IN_ENABLED | USB_OUT_ENABLED
                                      | USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);
            s_in_handle = 0;
            s_out_handle = HIDRxPacket(HID_EP, (uint8_t *)s_out_buffer, HID_INT_OUT_EP_SIZE);
            break;

        case EVENT_EP0_REQUEST:
            /* HID requests on the control endpoint, such as the report descriptor. */
            USBCheckHIDRequest();
            break;

        default:
            break;
    }
    return true;
}
