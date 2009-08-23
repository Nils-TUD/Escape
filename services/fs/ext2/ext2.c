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
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/heap.h>
#include <esc/proc.h>
#include "ext2.h"
#include "blockcache.h"
#include "inodecache.h"
#include "request.h"

bool ext2_init(sExt2 *e) {
	tFD fd;
	/* we have to try it multiple times in this case since the kernel loads ata and fs
	 * directly after another and we don't know who's ready first */
	/* TODO later the device for the root-partition should be chosen in the multiboot-parameters */
	do {
		fd = open("drivers:/hda1",IO_WRITE | IO_READ);
		if(fd < 0)
			yield();
	}
	while(fd < 0);

	e->ataFd = fd;
	if(!ext2_readSectors(e,(u8*)&(e->superBlock),2,1)) {
		close(e->ataFd);
		printe("Unable to read super-block");
		return false;
	}

	e->groups = (sBlockGroup*)malloc(BLOCK_SIZE(e));
	/* TODO determine block-number of group-table */
	if(!ext2_readBlocks(e,(u8*)e->groups,2,1)) {
		close(e->ataFd);
		printe("Unable to read group-table");
		return false;
	}

	/* init caches */
	ext2_icache_init(e);
	ext2_bcache_init(e);

	return true;
}

void ext2_destroy(sExt2 *e) {
	free(e->groups);
	close(e->ataFd);
}
