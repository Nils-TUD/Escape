/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SERVICE_H_
#define SERVICE_H_

/* the usable IRQs */
#define IRQ_TIMER					0x20
#define IRQ_KEYBOARD				0x21
#define IRQ_COM2					0x23
#define IRQ_COM1					0x24
#define IRQ_FLOPPY					0x26
#define IRQ_CMOS_RTC				0x28
#define IRQ_ATA1					0x2E
#define IRQ_ATA2					0x2F

#define SERVICE_TYPE_MULTIPIPE		0
#define SERVICE_TYPE_SINGLEPIPE		4

/**
 * Registers a service with given name.
 *
 * @param name the service-name. Should be alphanumeric!
 * @param type the service-type: SERVICE_TYPE_MULTIPIPE or SERVICE_TYPE_SINGLEPIPE
 * @return the service-id if successfull, < 0 if an error occurred
 */
tServ regService(const char *name,u8 type);

/**
 * Unregisters your service
 *
 * @param service the service-id
 * @return 0 on success or a negative error-code
 */
s32 unregService(tServ service);

/**
 * Looks wether a client wants to be served and returns a file-descriptor for it.
 *
 * @param services an array with service-ids to check
 * @param count the size of <services>
 * @param serv will be set to the service from which the client has been taken
 * @return the file-descriptor if successfull or the error-code
 */
tFD getClient(tServ *services,u32 count,tServ *serv);

#endif /* SERVICE_H_ */
