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
#include <stdlib.h>
#include <string.h>
#include "iso9660.h"
#include "direcache.h"
#include "rw.h"
#include "../blockcache.h"

void iso_direc_init(sISO9660 *h) {
	size_t i;
	for(i = 0; i < ISO_DIRE_CACHE_SIZE; i++)
		h->direcache[i].id = 0;
	h->direcNextFree = 0;
}

const sISOCDirEntry *iso_direc_get(sISO9660 *h,inode_t id) {
	const sISODirEntry *e;
	sCBlock *blk;
	block_t blockLBA;
	size_t i,blockSize,offset;
	int unused = -1;

	/* search in the cache */
	for(i = 0; i < ISO_DIRE_CACHE_SIZE; i++) {
		if(h->direcache[i].id == id)
			return (const sISOCDirEntry*)(h->direcache + i);
		if(unused < 0 && h->direcache[i].id == 0)
			unused = i;
	}

	/* overwrite an arbitrary one, if no one is free */
	if(unused < 0) {
		unused = h->direcNextFree;
		h->direcNextFree = (h->direcNextFree + 1) % ISO_DIRE_CACHE_SIZE;
	}

	if(id == ISO_ROOT_DIR_ID(h)) {
		e = (const sISODirEntry*)&h->primary.data.primary.rootDir;
		memcpy(&(h->direcache[unused].entry),e,sizeof(sISODirEntry));
		h->direcache[unused].id = id;
	}
	else {
		/* load it from disk */
		blockSize = ISO_BLK_SIZE(h);
		offset = id & (blockSize - 1);
		blockLBA = id / blockSize + offset / blockSize;
		blk = bcache_request(&h->blockCache,blockLBA,BMODE_READ);
		if(blk == NULL)
			return NULL;
		e = (const sISODirEntry*)((uintptr_t)blk->buffer + (offset % blockSize));
		/* don't copy the name! */
		memcpy(&(h->direcache[unused].entry),e,sizeof(sISODirEntry));
		h->direcache[unused].id = id;
		bcache_release(blk);
	}
	return (const sISOCDirEntry*)(h->direcache + unused);
}
