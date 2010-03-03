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

#define MAX_SNAME_LEN			50
#define MAX_SERVICE_PATH_LEN	255
#define MAX_WAIT_RETRIES		1000

typedef struct {
	/* the service-name */
	char *name;
	/* an array of service-names to wait for */
	u8 waitCount;
	char **waits;
	/* an array of dependencies to load before */
	u8 depCount;
	char **deps;
} sServiceLoad;

/**
 * Loads the given services
 *
 * @param loads the services to load
 * @return true if successfull
 */
bool loadServices(sServiceLoad **loads);

/**
 * Prints the given services
 *
 * @param loads the services to print
 */
void printServices(sServiceLoad **loads);

#endif /* SERVICE_H_ */
