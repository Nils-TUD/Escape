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

#include <mem/cache.h>
#include <mem/pagedir.h>
#include <mem/useraccess.h>
#include <mem/virtmem.h>
#include <task/proc.h>
#include <vfs/file.h>
#include <vfs/node.h>
#include <vfs/vfs.h>
#include <common.h>
#include <errno.h>
#include <spinlock.h>
#include <string.h>

VFSFile::VFSFile(const fs::User &u,VFSNode *p,char *n,uint m,bool &success)
		: VFSNode(u,n,m,success), dynamic(true), size(), direct(), indirect(), dblindir() {
	if(!success)
		return;
	append(p);
}

VFSFile::VFSFile(const fs::User &u,VFSNode *p,char *n,void *buffer,size_t len,uint m,bool &success)
		: VFSNode(u,n,m,success), dynamic(false), size(len), data(buffer) {
	if(!success)
		return;
	append(p);
}

VFSFile::~VFSFile() {
	if(dynamic)
		truncate(0);
}

off_t VFSFile::seek(off_t position,off_t offset,uint whence) const {
	switch(whence) {
		case SEEK_SET:
			return offset;
		case SEEK_CUR:
			/* we can't validate the position here because the content of virtual files may be
			 * generated on demand */
			return position + offset;
		default:
		case SEEK_END:
			return size;
	}
}

ssize_t VFSFile::getSize() {
	return size;
}

int VFSFile::reserve(off_t newSize) {
	if(!dynamic)
		return -ENOTSUP;

	LockGuard<SpinLock> g(&lock);
	newSize = esc::Util::round_page_up(newSize);
	for(off_t offset = 0; offset < newSize; offset += PAGE_SIZE) {
		if(!getBlock(offset,true))
			return -ENOMEM;
	}
	return 0;
}

void *VFSFile::getIndirBlock(void ***table,off_t offset,size_t blksize,bool alloc) {
	if(!*table) {
		if(!alloc)
			return NULL;
		frameno_t frame = PhysMem::allocate(PhysMem::KERN);
		if(frame == PhysMem::INVALID_FRAME)
			return NULL;
		*table = (void**)(DIR_MAP_AREA + (frame << PAGE_BITS));
		memclear(*table,PAGE_SIZE);
	}

	void *res = (*table)[offset / blksize];
	if(!res && alloc) {
		frameno_t frame = PhysMem::allocate(PhysMem::KERN);
		if(frame == PhysMem::INVALID_FRAME)
			return NULL;
		res = (*table)[offset / blksize] = (void*)(DIR_MAP_AREA + (frame << PAGE_BITS));
		memclear(res,PAGE_SIZE);
	}

	if(res && blksize > PAGE_SIZE) {
		size_t blksPerPage = PAGE_SIZE / sizeof(void*);
		return getIndirBlock((void***)&res,offset % blksize,blksize / blksPerPage,alloc);
	}
	return res;
}

void *VFSFile::getBlock(off_t offset,bool alloc) {
	if(!dynamic)
		return (void*)((uintptr_t)data + esc::Util::round_page_dn(offset));

	/* direct */
	if(offset < (off_t)(PAGE_SIZE * DIRECT_COUNT)) {
		void **table = (void**)&direct;
		return getIndirBlock(&table,offset,PAGE_SIZE,alloc);
	}

	/* indirect */
	size_t blksPerPage = PAGE_SIZE / sizeof(void*);
	offset -= PAGE_SIZE * DIRECT_COUNT;
	if(offset < (off_t)(blksPerPage * PAGE_SIZE))
		return getIndirBlock(&indirect,offset,PAGE_SIZE,alloc);

	offset -= blksPerPage * PAGE_SIZE;
	if(offset < (off_t)(DBL_INDIR_COUNT * blksPerPage * PAGE_SIZE))
		return getIndirBlock((void***)&dblindir,offset,blksPerPage * PAGE_SIZE,alloc);
	return NULL;
}

static void freeFrame(void *addr) {
	frameno_t frame = ((uintptr_t)addr - DIR_MAP_AREA) >> PAGE_BITS;
	PhysMem::free(frame,PhysMem::KERN);
}

static void doTruncate(void **table,size_t count,int level) {
	for(size_t i = 0; i < count; ++i) {
		if(table[i]) {
			if(level > 0)
				doTruncate((void**)table[i],PAGE_SIZE / sizeof(void*),level - 1);

			freeFrame(table[i]);
			table[i] = NULL;
		}
	}
}

int VFSFile::truncate(off_t length) {
	LockGuard<SpinLock> g(&lock);
	if(length > size) {
		size_t blksPerPage = PAGE_SIZE / sizeof(void*);
		if(length > (off_t)(DBL_INDIR_COUNT * blksPerPage * PAGE_SIZE))
			return -ENOMEM;
		size = length;
	}
	else if(length > 0)
		return -ENOTSUP;
	else {
		doTruncate(direct,DIRECT_COUNT,0);
		if(indirect) {
			doTruncate(indirect,PAGE_SIZE / sizeof(void*),0);
			freeFrame(indirect);
			indirect = NULL;

			if(dblindir) {
				doTruncate((void**)dblindir,VFSFile::DBL_INDIR_COUNT,1);
				freeFrame(dblindir);
				dblindir = NULL;
			}
		}
		size = 0;
	}
	return 0;
}

ssize_t VFSFile::read(A_UNUSED pid_t pid,A_UNUSED OpenFile *file,USER void *buffer,
                      off_t offset,size_t count) {
	LockGuard<SpinLock> g(&lock);
	if((off_t)(offset + count) < offset)
		return -EINVAL;
	if(offset > size)
		return 0;

	if((off_t)(offset + count) > size)
		count = size - offset;
	size_t rem = count;
	char *buf = reinterpret_cast<char*>(buffer);
	while(rem > 0) {
		size_t pageoff = offset & (PAGE_SIZE - 1);
		size_t amount = esc::Util::min((size_t)PAGE_SIZE - pageoff,rem);
		const char *block = (const char*)getBlock(offset,true);
		if(!block)
			return -ENOMEM;

		int res = UserAccess::write(buf,block + pageoff,amount);
		if(res < 0)
			return res;

		offset += amount;
		buf += amount;
		rem -= amount;
	}
	acctime = Timer::getTime();
	return count;
}

ssize_t VFSFile::write(A_UNUSED pid_t pid,A_UNUSED OpenFile *file,USER const void *buffer,
                       off_t offset,size_t count) {
	/* need to create cache? */
	LockGuard<SpinLock> g(&lock);
	if((off_t)(offset + count) < offset)
		return -EINVAL;

	size_t rem = count;
	const char *buf = reinterpret_cast<const char*>(buffer);
	while(rem > 0) {
		size_t pageoff = offset & (PAGE_SIZE - 1);
		size_t amount = esc::Util::min((size_t)PAGE_SIZE - pageoff,rem);
		char *block = (char*)getBlock(offset,true);
		if(!block)
			return -ENOMEM;

		int res = UserAccess::read(block + pageoff,buf,amount);
		if(res < 0)
			return res;

		offset += amount;
		buf += amount;
		rem -= amount;
	}
	size = esc::Util::max(size,offset);
	acctime = modtime = Timer::getTime();
	return count;
}

void VFSFile::print(OStream &os) const {
	os.writef("File '%s':\n",getPath());
	os.writef("  dynamic  : %s\n",dynamic ? "true" : "false");
	os.writef("  size     : %ld\n",size);
	if(dynamic) {
		for(size_t i = 0; i < VFSFile::DIRECT_COUNT; ++i)
			os.writef("  dir[%4zu]: %p\n",i,direct[i]);

		os.writef("  ind      : %p\n",indirect);
		if(indirect) {
			for(size_t i = 0; i < PAGE_SIZE / sizeof(void*); ++i)
				os.writef("  ind[%4zu]: %p\n",i,indirect[i]);
		}

		os.writef("  dbl      : %p\n",dblindir);
		if(dblindir) {
			for(size_t i = 0; i < DBL_INDIR_COUNT; ++i) {
				os.writef("  dbl[%4zu]: %p\n",i,dblindir[i]);
				void **table = dblindir[i];
				if(table) {
					for(size_t j = 0; j < PAGE_SIZE / sizeof(void*); ++j)
						os.writef("  dbl[%4zu][%4zu]: %p\n",i,j,table[j]);
				}
			}
		}
	}
	else
		os.writef("  data     : %p\n",data);
}
