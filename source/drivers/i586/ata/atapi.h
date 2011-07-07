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

#ifndef ATAPI_H_
#define ATAPI_H_

#include <esc/common.h>
#include "device.h"

/**
 * Reads from an ATAPI-device
 *
 * @param device the device
 * @param op the operation: just OP_READ here ;)
 * @param buffer the buffer to write to
 * @param lba the block-address to start at
 * @param secSize the size of a sector
 * @param secCount number of sectors
 * @return true on success
 */
bool atapi_read(sATADevice *device,uint op,void *buffer,uint64_t lba,size_t secSize,size_t secCount);

/**
 * Determines the capacity for the given device
 *
 * @param device the device
 * @return the capacity or 0 if failed
 */
size_t atapi_getCapacity(sATADevice *device);

#endif /* ATAPI_H_ */
