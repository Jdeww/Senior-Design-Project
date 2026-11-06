/*
 * module_manager.h - Keeps the chain running: polls each board for events, turns
 * them into reports for the PC, looks for new boards, and rebuilds the chain when
 * a board drops out.
 *
 * Main loop only. The system tick must be running with interrupts enabled first.
 */
#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

/* Call once at boot. Loads the database and registers the chain. */
void module_manager_init(void);

/* Call on every main loop pass. Does its own timing. */
void module_manager_task(void);

#endif /* MODULE_MANAGER_H */
