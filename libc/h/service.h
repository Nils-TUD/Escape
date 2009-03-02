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
 * @return the service-id if successfull, < 0 if an error occurred
 */
s32 regService(cstring name);

/**
 * Unregisters your service
 *
 * @param service the service-id
 * @return 0 on success or a negative error-code
 */
s32 unregService(s32 service);

/**
 * Waits for a client to serve
 *
 * @param service the service-id
 * @return the file-descriptor if successfull or the error-code
 */
s32 waitForClient(s32 service);

#endif /* SERVICE_H_ */
