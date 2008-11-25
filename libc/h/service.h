/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SERVICE_H_
#define SERVICE_H_

/**
 * Registers a service with given name.
 *
 * @param name the service-name. Should be alphanumeric!
 * @return 0 if successfull, < 0 if an error occurred
 */
s32 regService(cstring name);

/**
 * Unregisters your service
 */
void unregService(void);

#endif /* SERVICE_H_ */
