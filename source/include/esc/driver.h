/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#ifdef __i386__
#include <esc/arch/i586/driver.h>
#endif
#ifdef __eco32__
#include <esc/arch/eco32/driver.h>
#endif
#ifdef __mmix__
#include <esc/arch/mmix/driver.h>
#endif

#define DRV_OPEN					1
#define DRV_READ					2
#define DRV_WRITE					4
#define DRV_CLOSE					8
#define DRV_FS						16

#define GW_NOBLOCK					1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Registers a driver with given name.
 *
 * @param name the driver-name. Should be alphanumeric!
 * @param flags what functions do you want to implement (DRV_*) ?
 * @return the file-desc if successfull, < 0 if an error occurred
 */
int regDriver(const char *name,uint flags) A_CHECKRET;

/**
 * Fetches the client-id from the given file-descriptor
 *
 * @param fd the file-descriptor
 * @return the id or an error
 */
inode_t getClientId(int fd);

/**
 * Opens a file for the client with given client-id.
 *
 * @param fd the file-descriptor for the driver
 * @param cid the client-id
 * @return the file-descriptor or a negative error-code
 */
int getClient(int fd,inode_t cid) A_CHECKRET;

/**
 * For drivers: Looks whether a client wants to be served. If not and GW_NOBLOCK is not provided
 * it waits until a client should be served. if not and GW_NOBLOCK is enabled, it returns an error.
 * If a client wants to be served, the message is fetched from him and a file-descriptor is returned.
 * Note that you may be interrupted by a signal!
 *
 * @param fds an array with file-descs to check
 * @param fdCount the number of file-descs
 * @param drv will be set to the file-desc from which the client has been taken
 * @param mid will be set to the msg-id
 * @param msg the message
 * @param size the (max) size of the message
 * @param flags the flags
 * @return the file-descriptor for the communication with the client
 */
int getWork(int *fds,size_t fdCount,int *drv,msgid_t *mid,void *msg,size_t size,uint flags) A_CHECKRET;

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_H_ */
