/*
 * usb_app.h - Connects the MLA USB stack to the rest of the motherboard: sends the
 * report queue to the PC and carries out the PC's commands.
 *
 * Wiring it into the firmware:
 *   - call usb_app_init() at boot, before any long blocking work, so the PC can
 *     enumerate the device while the chain is still being registered
 *   - in the high-priority ISR: USBDeviceTasks();
 *   - call usb_app_task() on every main loop pass
 *
 * The stack itself runs in the interrupt, so the PC's control requests are
 * answered even while the main loop is blocked. Reports only move when
 * usb_app_task() runs, at most one per 1 ms frame.
 */
#ifndef USB_APP_H
#define USB_APP_H

#include <stdbool.h>

/* Starts the USB module and connects to the bus. Turns on interrupt priorities
 * and high-priority interrupts as a side effect. */
void usb_app_init(void);

/* Main loop only. */
void usb_app_task(void);

/* True once the PC has configured the device and the bus is not suspended. */
bool usb_app_ready(void);

#endif /* USB_APP_H */
