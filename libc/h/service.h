/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SERVICE_H_
#define SERVICE_H_

/* the usable IRQs */
#define IRQ_TIMER			0x20
#define IRQ_KEYBOARD		0x21
#define IRQ_COM2			0x23
#define IRQ_COM1			0x24
#define IRQ_FLOPPY			0x26
#define IRQ_CMOS_RTC		0x28
#define IRQ_ATA1			0x2E
#define IRQ_ATA2			0x2F

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
 * Adds an interrupt-listener
 *
 * @param service the node-id
 * @param irq the irq-number
 * @param msg the message
 * @param msgLen the message-length
 * @return 0 on success or a negative error-code
 */
s32 addIntrptListener(s32 service,u16 irq,void *msg,u32 msgLen);

/**
 * Removes an interrupt-listener
 *
 * @param service the node-id
 * @param irq the irq-number
 * @return 0 on success or a negative error-code
 */
s32 remIntrptListener(s32 service,u16 irq);

/**
 * Waits for a client to serve
 *
 * @param service the service-id
 * @return the file-descriptor if successfull or the error-code
 */
s32 waitForClient(s32 service);

#endif /* SERVICE_H_ */
