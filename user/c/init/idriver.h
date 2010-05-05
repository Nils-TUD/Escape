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
#include <esc/util/vector.h>

#define MAX_SNAME_LEN			50
#define MAX_DRIVER_PATH_LEN	255
#define MAX_WAIT_RETRIES		1000

typedef struct {
	/* the driver-name */
	char *name;
	/* an array of driver-names to wait for */
	sVector *waits;
	/* an array of dependencies to load before */
	sVector *deps;
} sDriverLoad;

/**
 * Loads the given drivers
 *
 * @param loads the drivers to load
 * @return true if successfull
 */
bool loadDrivers(sVector *loads);

/**
 * Prints the given drivers
 *
 * @param loads the drivers to print
 */
void printDrivers(sVector *loads);

#endif /* DRIVER_H_ */
