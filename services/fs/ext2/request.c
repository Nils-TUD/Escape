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
	tMsgId id;

	/* send read-request */

	msg.args.arg1 = e->drive;
	msg.args.arg2 = e->partition;
	msg.args.arg3 = lba >> 32;
	msg.args.arg4 = lba & 0xFFFFFFFF;
	msg.args.arg5 = secCount;
	if(send(e->ataFd,MSG_ATA_READ_REQ,&msg,sizeof(msg.args)) < 0) {
		printe("Unable to send request to ATA");
		return false;
	}

	/* read header */
	if(receive(e->ataFd,&id,&msg) < 0) {
		printe("Reading response-header from ATA failed (drive=%d, part=%d, lba=%x, secCount=%d)",
				e->drive,e->partition,(u32)lba,secCount);
		return false;
	}

	memcpy(buffer,msg.data.d,msg.data.arg1);
	return true;
}
