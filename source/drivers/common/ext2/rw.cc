/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <fs/fsdev.h>
#include <sys/common.h>
#include <sys/io.h>
#include <sys/messages.h>
#include <sys/thread.h>
#include <stdio.h>

#include "ext2.h"
#include "rw.h"

int Ext2RW::readSectors(Ext2FileSystem *e,void *buffer,uint64_t lba,size_t secCount) {
	off_t off = seek(e->fd,lba * DISK_SECTOR_SIZE,SEEK_SET);
	if(off < 0) {
		printe("Unable to seek to %x",lba * DISK_SECTOR_SIZE);
		return off;
	}

	ssize_t res = IGNSIGS(read(e->fd,buffer,secCount * DISK_SECTOR_SIZE));
	if(res != (ssize_t)(secCount * DISK_SECTOR_SIZE)) {
		printe("Unable to read %d sectors @ %x",secCount,lba * DISK_SECTOR_SIZE);
		return res;
	}

	return 0;
}

int Ext2RW::writeSectors(Ext2FileSystem *e,const void *buffer,uint64_t lba,size_t secCount) {
	off_t off = seek(e->fd,lba * DISK_SECTOR_SIZE,SEEK_SET);
	if(off < 0) {
		printe("Unable to seek to %x",lba * DISK_SECTOR_SIZE);
		return off;
	}

	ssize_t res = write(e->fd,buffer,secCount * DISK_SECTOR_SIZE);
	if(res != (ssize_t)(secCount * DISK_SECTOR_SIZE)) {
		printe("Unable to write %d sectors @ %x",secCount,lba * DISK_SECTOR_SIZE);
		return res;
	}

	return 0;
}
