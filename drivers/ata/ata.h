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

#ifndef ATA_H_
#define ATA_H_

#include <esc/common.h>
#include <esc/debug.h>
#include <stdio.h>
#include "device.h"

/* for printing debug-infos */
#define ATA_PR1(fmt,...)	/*do { \
		printf("[ATA] "); \
		printf(fmt,## __VA_ARGS__); \
		printf("\n"); \
		fflush(stdout); \
	} while(0);*/

#define ATA_PR2(fmt,...)	/*do { \
		printf("[ATA] "); \
		printf(fmt,## __VA_ARGS__); \
		printf("\n"); \
		fflush(stdout); \
	} while(0);*/

#define ATA_LOG(fmt,...)	do { \
		printf("[ATA] "); \
		printf(fmt,## __VA_ARGS__); \
		printf("\n"); \
		fflush(stdout); \
	} while(0);

/**
 * Reads or writes from/to an ATA-device
 *
 * @param device the device
 * @param opWrite true if writing
 * @param buffer the buffer to write to
 * @param lba the block-address to start at
 * @param secSize the size of a sector
 * @param secCount number of sectors
 * @return true on success
 */
bool ata_readWrite(sATADevice *device,bool opWrite,u16 *buffer,u64 lba,u16 secSize,u16 secCount);

bool ata_transferPIO(sATADevice *device,bool opWrite,u16 *buffer,u16 secSize,u16 secCount,bool waitFirst);
bool ata_transferDMA(sATADevice *device,bool opWrite,u16 *buffer,u16 secSize,u16 secCount);

#endif /* ATA_H_ */
