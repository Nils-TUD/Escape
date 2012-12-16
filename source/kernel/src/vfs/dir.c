/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/mem/vmm.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/dir.h>
#include <sys/vfs/link.h>
#include <sys/vfs/node.h>
#include <sys/vfs/info.h>
#include <sys/vfs/fsmsgs.h>
#include <esc/fsinterface.h>
#include <esc/endian.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

/* VFS-directory-entry (equal to the direntry of ext2) */
typedef struct {
	inode_t nodeNo;
	uint16_t recLen;
	uint16_t nameLen;
	/* name follows (up to 255 bytes) */
} A_PACKED sVFSDirEntry;

static ssize_t vfs_dir_read(pid_t pid,sFile *file,sVFSNode *node,void *buffer,off_t offset,
		size_t count);
static off_t vfs_dir_seek(pid_t pid,sVFSNode *node,off_t position,off_t offset,uint whence);

sVFSNode *vfs_dir_create(pid_t pid,sVFSNode *parent,char *name) {
	sVFSNode *target;
	sVFSNode *node = vfs_node_create(pid,name);
	if(node == NULL)
		return NULL;

	node->mode = DIR_DEF_MODE;
	if(vfs_link_create(pid,node,(char*)".",node) == NULL) {
		vfs_node_destroy(node);
		return NULL;
	}
	/* the root-node has no parent */
	target = parent == NULL ? node : parent;
	if(vfs_link_create(pid,node,(char*)"..",target) == NULL) {
		vfs_node_destroy(node);
		return NULL;
	}

	node->read = vfs_dir_read;
	node->write = NULL;
	node->seek = vfs_dir_seek;
	node->destroy = NULL;
	node->close = NULL;
	vfs_node_append(parent,node);
	return node;
}

static off_t vfs_dir_seek(A_UNUSED pid_t pid,A_UNUSED sVFSNode *node,off_t position,
		off_t offset,uint whence) {
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

static ssize_t vfs_dir_read(pid_t pid,A_UNUSED sFile *file,sVFSNode *node,USER void *buffer,
		off_t offset,size_t count) {
	size_t byteCount,fsByteCount;
	void *fsBytes = NULL,*fsBytesDup;
	sVFSNode *n,*firstChild;
	bool isValid;
	assert(node != NULL);
	assert(buffer != NULL);

	/* the root-directory is distributed on the fs-device and the kernel */
	/* therefore we have to read it from the fs-device, too */
	/* but don't do that if we're the kernel (vfsr does not work then) */
	byteCount = 0;
	fsByteCount = 0;
	if(node->parent == NULL && pid != KERNEL_PID) {
		const size_t bufSize = 1024;
		size_t c,curSize = bufSize;
		fsBytes = cache_alloc(bufSize);
		if(fsBytes != NULL) {
			sFile *rfile;
			thread_addHeapAlloc(fsBytes);
			if(vfs_fsmsgs_openPath(pid,VFS_READ,"/",&rfile) == 0) {
				while((c = vfs_readFile(pid,rfile,(uint8_t*)fsBytes + fsByteCount,bufSize)) > 0) {
					fsByteCount += c;
					if(c < bufSize)
						break;

					curSize += bufSize;
					fsBytesDup = cache_realloc(fsBytes,curSize);
					if(fsBytesDup == NULL) {
						byteCount = 0;
						goto error;
					}
					thread_remHeapAlloc(fsBytes);
					fsBytes = fsBytesDup;
					thread_addHeapAlloc(fsBytes);
				}
				vfs_closeFile(pid,rfile);
			}
			thread_remHeapAlloc(fsBytes);
		}
		byteCount += fsByteCount;
	}

	firstChild = n = vfs_node_openDir(node,true,&isValid);
	if(isValid) {
		/* count the number of bytes in the virtual directory */
		/* note that we do that here because using the fs service involves context-switches,
		 * which may lead to a deadlock if we hold the node-lock during that time */
		while(n != NULL) {
			if(node->parent != NULL || ((n->nameLen != 1 || strcmp(n->name,".") != 0)
					&& (n->nameLen != 2 || strcmp(n->name,"..") != 0)))
				byteCount += sizeof(sVFSDirEntry) + n->nameLen;
			n = n->next;
		}

		/* now allocate mem on the heap and copy all data into it */
		fsBytesDup = cache_realloc(fsBytes,byteCount);
		if(fsBytesDup == NULL)
			byteCount = 0;
		else {
			size_t len;
			sVFSDirEntry *dirEntry = (sVFSDirEntry*)((uint8_t*)fsBytesDup + fsByteCount);
			fsBytes = fsBytesDup;
			thread_addHeapAlloc(fsBytes);
			n = firstChild;
			while(n != NULL) {
				if(node->parent == NULL && ((n->nameLen == 1 && strcmp(n->name,".") == 0) ||
						(n->nameLen == 2 && strcmp(n->name,"..") == 0))) {
					n = n->next;
					continue;
				}
				len = n->nameLen;
				/* unfortunatly, we have to convert the endianess here, because readdir() expects
				 * that its encoded in little endian */
				dirEntry->nodeNo = cputole32(vfs_node_getNo(n));
				dirEntry->nameLen = cputole16(len);
				dirEntry->recLen = cputole16(sizeof(sVFSDirEntry) + len);
				memcpy(dirEntry + 1,n->name,len);
				dirEntry = (sVFSDirEntry*)((uint8_t*)dirEntry + sizeof(sVFSDirEntry) + len);
				n = n->next;
			}
			thread_remHeapAlloc(fsBytes);
		}
	}
	vfs_node_closeDir(node,true);

	if(offset > (off_t)byteCount)
		offset = byteCount;
	byteCount = MIN(byteCount - offset,count);
	if(byteCount > 0) {
		thread_addHeapAlloc(fsBytes);
		memcpy(buffer,(uint8_t*)fsBytes + offset,byteCount);
		thread_remHeapAlloc(fsBytes);
	}
error:
	cache_free(fsBytes);
	return byteCount;
}
