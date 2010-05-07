/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef DRIVER_H_
#define DRIVER_H_

#include <esc/common.h>

/* the usable IRQs */
#define IRQ_TIMER					0x20
#define IRQ_KEYBOARD				0x21
#define IRQ_COM2					0x23
#define IRQ_COM1					0x24
#define IRQ_FLOPPY					0x26
#define IRQ_CMOS_RTC				0x28
#define IRQ_ATA1					0x2E
#define IRQ_ATA2					0x2F

#define DRV_OPEN					1
#define DRV_READ					2
#define DRV_WRITE					4
#define DRV_CLOSE					8
#define DRV_FS						16
#define DRV_TERM					32

#define GW_NOBLOCK					1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Registers a driver with given name.
 *
 * @param name the driver-name. Should be alphanumeric!
 * @param flags what functions do you want to implement (DRV_*) ?
 * @return the driver-id if successfull, < 0 if an error occurred
 */
tDrvId regDriver(const char *name,u32 flags) A_CHECKRET;

/**
 * Unregisters your driver
 *
 * @param driver the driver-id
 * @return 0 on success or a negative error-code
 */
s32 unregDriver(tDrvId driver);

/**
 * For drivers: Sets whether currently data is readable or not
 *
 * @param driver the driver-id
 * @param readable whether there is data to read
 * @return 0 on success
 */
s32 setDataReadable(tDrvId driver,bool readable);

/**
 * Opens a file for the client with given thread-id.
 *
 * @param id the driver-id
 * @param tid the client-thread-id
 * @return the file-descriptor or a negative error-code
 */
tFD getClientThread(tDrvId id,tTid tid) A_CHECKRET;

/**
 * For drivers: Looks whether a client wants to be served. If not and GW_NOBLOCK is not provided
 * it waits until a client should be served. if not and GW_NOBLOCK is enabled, it returns an error.
 * If a client wants to be served, the message is fetched from him and a file-descriptor is returned.
 * Note that you may be interrupted by a signal!
 *
 * @param ids an array with driver-ids to check
 * @param idCount the number of driver-ids
 * @param drv will be set to the driver from which the client has been taken
 * @param mid will be set to the msg-id
 * @param msg the message
 * @param size the (max) size of the message
 * @param flags the flags
 * @return the file-descriptor for the communication with the client
 */
tFD getWork(tDrvId *ids,u32 idCount,tDrvId *drv,tMsgId *mid,void *msg,u32 size,u8 flags) A_CHECKRET;

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_H_ */
