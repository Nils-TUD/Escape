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

#include <fs/blockcache.h>
#include <sys/common.h>
#include <stdlib.h>
#include <string.h>

#include "direcache.h"
#include "iso9660.h"
#include "rw.h"

ISO9660DirCache::ISO9660DirCache(ISO9660FileSystem *h)
	: _nextFree(0), _cache(new ISOCDirEntry[ISO_DIRE_CACHE_SIZE]()), _fs(h) {
}

const ISOCDirEntry *ISO9660DirCache::get(ino_t id) {
	const ISODirEntry *e;
	fs::CBlock *blk;
	block_t blockLBA;
	size_t i,blockSize,offset;
	int unused = -1;

	/* search in the cache */
	for(i = 0; i < ISO_DIRE_CACHE_SIZE; i++) {
		if(_cache[i].id == id)
			return _cache + i;
		if(unused < 0 && _cache[i].id == 0)
			unused = i;
	}

	/* overwrite an arbitrary one, if no one is free */
	if(unused < 0) {
		unused = _nextFree;
		_nextFree = (_nextFree + 1) % ISO_DIRE_CACHE_SIZE;
	}

	if(id == _fs->rootDirId()) {
		e = (const ISODirEntry*)&_fs->primary.data.primary.rootDir;
		memcpy(&(_cache[unused].entry),e,sizeof(ISODirEntry));
		_cache[unused].id = id;
	}
	else {
		/* load it from disk */
		blockSize = _fs->blockSize();
		offset = id & (blockSize - 1);
		blockLBA = id / blockSize + offset / blockSize;
		blk = _fs->blockCache.request(blockLBA,fs::BlockCache::READ);
		if(blk == NULL)
			return NULL;
		e = (const ISODirEntry*)((uintptr_t)blk->buffer + (offset % blockSize));
		/* don't copy the name! */
		memcpy(&(_cache[unused].entry),e,sizeof(ISODirEntry));
		_cache[unused].id = id;
		_fs->blockCache.release(blk);
	}
	return _cache + unused;
}

void ISO9660DirCache::print(FILE *f) {
	size_t i,freeEntries = 0;
	for(i = 0; i < ISO_DIRE_CACHE_SIZE; i++) {
		if(_cache[i].id == 0)
			freeEntries++;
	}
	fprintf(f,"\tTotal entries: %zu\n",ISO_DIRE_CACHE_SIZE);
	fprintf(f,"\tUsed entries: %zu\n",ISO_DIRE_CACHE_SIZE - freeEntries);
}
