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

#include <esc/common.h>
#include <messages.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/proc.h>
#include "request.h"
#include "ext2.h"

static sMsg msg;

bool ext2_readBlocks(sExt2 *e,u8 *buffer,u32 start,u16 blockCount) {
	return ext2_readSectors(e,buffer,BLOCKS_TO_SECS(e,start),BLOCKS_TO_SECS(e,blockCount));
}

bool ext2_readSectors(sExt2 *e,u8 *buffer,u64 lba,u16 secCount) {
	if(seek(e->ataFd,lba * SECTOR_SIZE) < 0) {
		printe("Unable to seek to %x\n",lba * SECTOR_SIZE);
		return false;
	}
	if(read(e->ataFd,buffer,secCount * SECTOR_SIZE) != secCount * SECTOR_SIZE) {
		printe("Unable to read %d sectors @ %x\n",secCount,lba * SECTOR_SIZE);
		return false;
	}

	return true;
}
