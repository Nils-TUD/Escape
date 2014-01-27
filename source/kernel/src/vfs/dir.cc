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

#include <sys/common.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/dir.h>
#include <sys/vfs/link.h>
#include <sys/vfs/node.h>
#include <sys/vfs/info.h>
#include <sys/vfs/fs.h>
#include <sys/vfs/openfile.h>
#include <esc/fsinterface.h>
#include <esc/endian.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

VFSDir::VFSDir(pid_t pid,VFSNode *p,char *n,mode_t m,bool &success)
		: VFSNode(pid,n,DIR_DEF_MODE | (m & 0777),success) {
	VFSNode *l;
	if(!success)
		return;

	if(!(l = new VFSLink(pid,this,(char*)".",this,success))) {
		success = false;
		return;
	}
	VFSNode::release(l);
	/* the root-node has no parent */
	VFSNode *target = p == NULL ? this : p;
	if(!(l = new VFSLink(pid,this,(char*)"..",target,success))) {
		success = false;
		return;
	}
	VFSNode::release(l);

	append(p);
}

off_t VFSDir::seek(A_UNUSED pid_t pid,off_t position,off_t offset,uint whence) const {
	switch(whence) {
		case SEEK_SET:
			return offset;
		case SEEK_CUR:
			/* we can't validate the position here because the content of virtual files may be
			 * generated on demand */
			return position + offset;
		default:
		case SEEK_END:
			return 0;
	}
}

size_t VFSDir::getSize(A_UNUSED pid_t pid) const {
	bool valid;
	size_t byteCount = 0;
	/* node is already locked */
	const VFSNode *n = openDir(true,&valid);
	while(n != NULL) {
		byteCount += sizeof(VFSDirEntry) + n->nameLen;
		n = n->next;
	}
	closeDir(true);
	return byteCount;
}

ssize_t VFSDir::read(A_UNUSED pid_t pid,A_UNUSED OpenFile *file,USER void *buffer,
		off_t offset,size_t count) {
	VFSDirEntry *fsBytes = NULL;
	const VFSNode *n,*first;
	assert(buffer != NULL);
	size_t byteCount = 0;

	bool valid;
	first = n = openDir(true,&valid);
	if(valid) {
		/* count the number of bytes in the virtual directory */
		/* note that we do that here because using the fs service involves context-switches,
		 * which may lead to a deadlock if we hold the node-lock during that time */
		while(n != NULL) {
			if(parent != NULL || ((n->nameLen != 1 || strcmp(n->name,".") != 0)
					&& (n->nameLen != 2 || strcmp(n->name,"..") != 0)))
				byteCount += sizeof(VFSDirEntry) + n->nameLen;
			n = n->next;
		}

		/* now allocate mem on the heap and copy all data into it */
		fsBytes = (VFSDirEntry*)Cache::alloc(byteCount);
		if(fsBytes == NULL)
			byteCount = 0;
		else {
			VFSDirEntry *dirEntry = fsBytes;
			Thread::addHeapAlloc(fsBytes);
			Thread::addLock(&VFSNode::treeLock);
			n = first;
			while(n != NULL) {
				if(parent == NULL && ((n->nameLen == 1 && strcmp(n->name,".") == 0) ||
						(n->nameLen == 2 && strcmp(n->name,"..") == 0))) {
					n = n->next;
					continue;
				}
				size_t len = n->nameLen;
				/* unfortunatly, we have to convert the endianess here, because readdir() expects
				 * that its encoded in little endian */
				dirEntry->nodeNo = cputole32(n->getNo());
				dirEntry->nameLen = cputole16(len);
				dirEntry->recLen = cputole16(sizeof(VFSDirEntry) + len);
				memcpy(dirEntry + 1,n->name,len);
				dirEntry = (VFSDirEntry*)((uint8_t*)dirEntry + sizeof(VFSDirEntry) + len);
				n = n->next;
			}
			Thread::remLock(&VFSNode::treeLock);
			Thread::remHeapAlloc(fsBytes);
		}
	}
	closeDir(true);

	if(offset > (off_t)byteCount)
		offset = byteCount;
	byteCount = MIN(byteCount - offset,count);
	if(byteCount > 0) {
		Thread::addHeapAlloc(fsBytes);
		memcpy(buffer,(uint8_t*)fsBytes + offset,byteCount);
		Thread::remHeapAlloc(fsBytes);
	}
	Cache::free(fsBytes);
	return byteCount;
}
