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

#ifndef APPS_H_
#define APPS_H_

#include <common.h>
#include <app.h>

/**
 * Initializes the apps-db
 */
void apps_init(void);

/**
 * Loads the database from the given file
 *
 * @param path the path to the db
 * @return 0 on success
 */
s32 apps_loadDBFromFile(const char *path);

/**
 * Loads the database from given definitions. The string HAS TO be null-terminated!
 *
 * @param defs the definitions
 * @return 0 on success
 */
s32 apps_loadDB(const char *defs);

/**
 * @return wether the application-permission-system is enabled
 */
bool apps_isEnabled(void);

/**
 * Wether the given application can use the given port-range
 *
 * @param app the app
 * @param start the start of the range
 * @param count the number of ports to request
 * @return true if so
 */
bool apps_canUseIOPorts(sApp *app,u16 start,u16 count);

/**
 * Wether the given application can use the driver with given name or type for <ops>
 *
 * @param app the app
 * @param name the driver-name
 * @param type the driver-type
 * @param ops the operations to perform (DRV_OP_*)
 * @return true if so
 */
bool apps_canUseDriver(sApp *app,const char *name,u32 type,u16 ops);

/**
 * Wether the given application can use the given service
 *
 * @param app the app
 * @param name the service-name
 * @return true if so
 */
bool apps_canUseService(sApp *app,const char *name);

/**
 * Wether the given application can use the given physical-memory-range
 *
 * @param app the app
 * @param start the start of the range
 * @param count the number of bytes to request
 * @return true if so
 */
bool apps_canUsePhysMem(sApp *app,u32 start,u32 count);

/**
 * Wether the given application can create shared memory with given name
 *
 * @param app the application
 * @param name the shmem-name
 * @return true if so
 */
bool apps_canCreateShMem(sApp *app,const char *name);

/**
 * Wether the given application can join the shared memory with given name
 *
 * @param app the application
 * @param name the shmem-name
 * @return true if so
 */
bool apps_canJoinShMem(sApp *app,const char *name);

/**
 * Wether the given application can use the FS for given operations
 *
 * @param app the application
 * @param ops the operations (FS_OP_*)
 * @return true if so
 */
bool apps_canUseFS(sApp *app,u16 ops);

/**
 * Wether the given application can announce a signal-handler for given interrupt
 *
 * @param app the application
 * @param signal the signal
 * @return true if so
 */
bool apps_canGetIntrpt(sApp *app,tSig signal);

/**
 * Determines the application with given name
 *
 * @param name the app-name
 * @return the application or NULL
 */
sApp *apps_get(const char *name);

#endif /* APPS_H_ */
