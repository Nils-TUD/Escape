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

#ifndef SERVICE_H_
#define SERVICE_H_

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

#define SERV_DEFAULT				1
#define SERV_FS						2
#define SERV_DRV_BINPRIV			4
#define SERV_DRV_BINPUB				8
#define SERV_DRV_TXTPRIV			16
#define SERV_DRV_TXTPUB				32

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Registers a service with given name.
 *
 * @param name the service-name. Should be alphanumeric!
 * @param type the service-type: SERV_*
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
 * For drivers: Sets wether currently data is readable or not
 *
 * @param service the service-id
 * @param readable wether there is data to read
 * @return 0 on success
 */
s32 setDataReadable(tServ service,bool readable);

/**
 * Looks wether a client wants to be served and returns a file-descriptor for it.
 *
 * @param services an array with service-ids to check
 * @param count the size of <services>
 * @param serv will be set to the service from which the client has been taken
 * @return the file-descriptor if successfull or the error-code
 */
tFD getClient(tServ *services,u32 count,tServ *serv);

/**
 * Opens a file for the client with given thread-id. Works just for multipipe-services!
 *
 * @param id the service-id
 * @param tid the client-thread-id
 * @return the file-descriptor or a negative error-code
 */
tFD getClientThread(tServ id,tTid tid);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_H_ */
