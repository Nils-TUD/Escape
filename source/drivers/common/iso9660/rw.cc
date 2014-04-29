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

#include <esc/common.h>
#include <esc/io.h>
#include <esc/thread.h>
#include <fs/fsdev.h>
#include <stdio.h>

#include "iso9660.h"
#include "rw.h"

int ISO9660RW::readSectors(ISO9660FileSystem *fs,void *buffer,uint64_t lba,size_t secCount) {
	ssize_t res;
	off_t off;
	int fd = fs->fd;
	if((off = seek(fd,lba * ATAPI_SECTOR_SIZE,SEEK_SET)) < 0) {
		printe("Unable to seek to %x",lba * ATAPI_SECTOR_SIZE);
		return off;
	}
	res = IGNSIGS(read(fd,buffer,secCount * ATAPI_SECTOR_SIZE));
	if(res != (ssize_t)secCount * ATAPI_SECTOR_SIZE) {
		printe("Unable to read %d sectors @ %x: %zd",secCount,lba * ATAPI_SECTOR_SIZE,res);
		return res;
	}
	return 0;
}
