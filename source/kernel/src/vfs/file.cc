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

VFSFile::VFSFile(pid_t pid,VFSNode *p,char *n,mode_t m,bool &success)
		: VFSNode(pid,n,S_IFREG | (m & MODE_PERM),success), dynamic(true), size(), pos(), data() {
	if(!success)
		return;
	append(p);
}

VFSFile::VFSFile(pid_t pid,VFSNode *p,char *n,void *buffer,size_t len,bool &success)
		: VFSNode(pid,n,FILE_DEF_MODE,success), dynamic(false), size(), pos(len), data(buffer) {
	if(!success)
		return;
	append(p);
}

VFSFile::~VFSFile() {
	if(dynamic)
		Cache::free(data);
	data = NULL;
}

off_t VFSFile::seek(A_UNUSED pid_t pid,off_t position,off_t offset,uint whence) const {
	switch(whence) {
		case SEEK_SET:
			return offset;
		case SEEK_CUR:
			/* we can't validate the position here because the content of virtual files may be
			 * generated on demand */
			return position + offset;
		default:
		case SEEK_END:
			return pos;
	}
}

int VFSFile::reserve(size_t newSize) {
	LockGuard<SpinLock> g(&lock);
	return doReserve(newSize);
}

int VFSFile::doReserve(size_t newSize) {
	if(!dynamic)
		return -ENOTSUP;
	/* ensure that we allocate enough memory */
	if(newSize > VFS::MAX_FILE_SIZE)
		return -ENOMEM;
	newSize = esc::Util::min(esc::Util::max(newSize,size * 2),VFS::MAX_FILE_SIZE);
	void *newData = Cache::realloc(data,newSize);
	if(!newData)
		return -ENOMEM;
	data = newData;
	size = newSize;
	return 0;
}

ssize_t VFSFile::getSize(A_UNUSED pid_t pid) {
	return pos;
}

ssize_t VFSFile::read(A_UNUSED pid_t pid,A_UNUSED OpenFile *file,USER void *buffer,
                      off_t offset,size_t count) {
	LockGuard<SpinLock> g(&lock);
	size_t byteCount = 0;
	if(data != NULL) {
		if(offset > pos)
			offset = pos;
		byteCount = esc::Util::min((size_t)(pos - offset),count);
		if(byteCount > 0) {
			int res = UserAccess::write(buffer,(uint8_t*)data + offset,byteCount);
			if(res < 0)
				return res;
		}
	}
	acctime = Timer::getTime();
	return byteCount;
}

ssize_t VFSFile::write(A_UNUSED pid_t pid,A_UNUSED OpenFile *file,USER const void *buffer,
                       off_t offset,size_t count) {
	int res;
	/* need to create cache? */
	LockGuard<SpinLock> g(&lock);
	if(data == NULL || size < offset + count) {
		res = doReserve(esc::Util::max(offset + count,VFS_INITIAL_WRITECACHE));
		if(res < 0)
			return res;
	}

	/* copy the data into the cache; this may segfault, which will leave the the state of the
	 * file as it was before, except that we've increased the buffer-size */
	if((res = UserAccess::read((uint8_t*)data + offset,buffer,count)) < 0)
		return res;

	/* we have checked size for overflow. so it is ok here */
	pos = esc::Util::max(pos,(off_t)(offset + count));
	acctime = modtime = Timer::getTime();
	return count;
}

int VFSFile::truncate(A_UNUSED pid_t pid,off_t length) {
	if(pos < length) {
		if(size < (size_t)length) {
			void *ndata = Cache::realloc(data,length);
			if(!ndata)
				return -ENOMEM;
			data = ndata;
			size = length;
		}
		memset((char*)data + pos,0,length - pos);
	}
	pos = length;
	return 0;
}
