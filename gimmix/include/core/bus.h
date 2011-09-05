/**
 * $Id: bus.h 218 2011-06-04 15:46:40Z nasmussen $
 */

#ifndef BUS_H_
#define BUS_H_

#include "common.h"

// address-space layout
#define DEV_CLASS_MASK		0x7FFF000000000000
#define DEV_NUMBER_MASK		0x0000FFFF00000000
#define DEV_REG_MASK		0x00000000FFFFFFFF
#define DEV_REG_SPACE		0x0000000100000000
#define DEV_START_ADDR		0x0001000000000000

#define CLASS_NO(addr)		((addr & DEV_CLASS_MASK) >> 48)
#define DEV_NO(addr)		((addr & DEV_NUMBER_MASK) >> 32)

// the device-interface
typedef octa (*fDevRead)(octa addr,bool sideEffects);
typedef void (*fDevWrite)(octa addr,octa data);
typedef void (*fDevReset)(void);
typedef void (*fDevShutdown)(void);

// describes a device
typedef struct {
	const char *name;
	octa startAddr;
	octa endAddr;
	octa irqMask;
	fDevReset reset;
	fDevShutdown shutdown;
	fDevRead read;
	fDevWrite write;
} sDevice;

/**
 * Initializes the bus. Will register all devices.
 *
 * @throws any kind of exception, that can be thrown by the device-init-functions
 */
void bus_init(void);

/**
 * Registers the given device
 *
 * @param dev the device
 */
void bus_register(const sDevice *dev);

/**
 * Resets all devices on the bus
 *
 * @throws any kind of exception, that can be thrown by the device-reset-functions
 */
void bus_reset(void);

/**
 * Shuts all devices on the bus down
 */
void bus_shutdown(void);

/**
 * @return a mask with all interrupt-bits of the available devices
 */
octa bus_getIntrptBits(void);

/**
 * @param index the index of the device
 * @return the device with given index or NULL
 */
const sDevice *bus_getDevByIndex(int index);

/**
 * @param addr the physical address
 * @return the device that is responsible for the given address or NULL if there is none
 */
const sDevice *bus_getDevByAddr(octa addr);

/**
 * Reads an octa from the given address. Searches for the responsible device and passes the read-
 * request to that device.
 *
 * @param addr the physical address
 * @param sideEffects whether side-effects are desired (in this case: events)
 * @throws TRAP_NONEX_MEMORY if the memory at <addr> does not exist or is not readable
 */
octa bus_read(octa addr,bool sideEffects);

/**
 * Writes an octa to the given address. Searches for the responsible device and passes the write-
 * request to that device.
 *
 * @param addr the physical address
 * @param data the octa to write
 * @param sideEffects whether side-effects are desired (in this case: events)
 * @throws TRAP_NONEX_MEMORY if the memory at <addr> does not exist or is not writable
 */
void bus_write(octa addr,octa data,bool sideEffects);

#endif /* BUS_H_ */
