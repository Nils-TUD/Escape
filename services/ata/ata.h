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
#include "drive.h"

/**
 * Announces signal-handler for the ATA-interrupts
 */
void ata_init(void);

/**
 * Sets the interrupt-waiting-flag to false
 */
void ata_unsetIntrpt(void);

/**
 * Waits until the interrupt-waiting-flag is set
 */
void ata_waitIntrpt(void);

/**
 * Waits a bit. Can be used after switching the drive
 */
void ata_wait(sATADrive *drive);

/**
 * Reads or writes from/to an ATA-drive
 *
 * @param drive the drive
 * @param opWrite true if writing
 * @param buffer the buffer to write to
 * @param lba the block-address to start at
 * @param secCount number of sectors
 * @return true on success
 */
bool ata_readWrite(sATADrive *drive,bool opWrite,u16 *buffer,u64 lba,u16 secCount);

#endif /* ATA_H_ */
