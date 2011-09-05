/**
 * $Id: sim.h 172 2011-04-15 09:41:38Z nasmussen $
 */

#ifndef SIM_H_
#define SIM_H_

/**
 * Initializes the simulator. It initializes all modules and prepares everything (loads ROM, ...)
 *
 * @throws any kind of exception that may be thrown by the module-init-functions
 */
void sim_init(void);

/**
 * Resets the simulator. It resets all modules and re-prepares everything (loads ROM again, ...)
 *
 * @throws any kind of exception that may be thrown by the module-reset-functions
 */
void sim_reset(void);

/**
 * Shuts the simulator down. It shuts down all modules.
 */
void sim_shutdown(void);

/**
 * Reports the given error and exits the simulator. sim_shutdown() will be called, so that
 * everything is cleaned up correctly.
 *
 * @param msg the message
 */
void sim_error(const char *msg,...);

#endif /* SIM_H_ */
