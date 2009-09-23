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

#ifndef ATAPI_H_
#define ATAPI_H_

#include <esc/common.h>
#include "drive.h"

/**
 * Reads from an ATAPI-drive
 *
 * @param drive the drive
 * @param opWrite true if writing (not supported here ;))
 * @param buffer the buffer to write to
 * @param lba the block-address to start at
 * @param secCount number of sectors
 * @return true on success
 */
bool atapi_read(sATADrive *drive,bool opWrite,u16 *buffer,u64 lba,u16 secCount);

/**
 * Determines the capacity for the given drive
 *
 * @param drive the drive
 * @return the capacity or 0 if failed
 */
u32 atapi_getCapacity(sATADrive *drive);

#endif /* ATAPI_H_ */
