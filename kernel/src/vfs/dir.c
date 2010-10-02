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

#include <sys/common.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/dir.h>
#include <sys/vfs/link.h>
#include <sys/vfs/node.h>
#include <sys/vfs/info.h>
#include <sys/vfs/real.h>
#include <esc/fsinterface.h>
#include <assert.h>
#include <string.h>

/* VFS-directory-entry (equal to the direntry of ext2) */
typedef struct {
	tInodeNo nodeNo;
	u16 recLen;
	u16 nameLen;
	/* name follows (up to 255 bytes) */
} A_PACKED sVFSDirEntry;

static s32 vfs_dir_seek(tPid pid,sVFSNode *node,s32 position,s32 offset,u32 whence);

sVFSNode *vfs_dir_create(tPid pid,sVFSNode *parent,char *name) {
	sVFSNode *target;
	sVFSNode *node = vfs_node_create(parent,name);
	if(node == NULL)
		return NULL;
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

	node->mode = MODE_TYPE_DIR | MODE_OWNER_READ | MODE_OWNER_WRITE | MODE_OWNER_EXEC |
		MODE_OTHER_READ | MODE_OTHER_EXEC;
	node->readHandler = vfs_dir_read;
	node->writeHandler = NULL;
	node->seek = vfs_dir_seek;
	node->destroy = NULL;
	return node;
}

static s32 vfs_dir_seek(tPid pid,sVFSNode *node,s32 position,s32 offset,u32 whence) {
	UNUSED(pid);
	UNUSED(node);
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

s32 vfs_dir_read(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	UNUSED(file);
	u32 byteCount,fsByteCount;
	u8 *fsBytes = NULL,*fsBytesDup;
	sVFSNode *n;
	assert(node != NULL);
	assert(buffer != NULL);

	/* we need the number of bytes first */
	n = vfs_node_getFirstChild(node);
	byteCount = 0;
	fsByteCount = 0;
	while(n != NULL) {
		if(node->parent != NULL || (strcmp(n->name,".") != 0 && strcmp(n->name,"..") != 0))
			byteCount += sizeof(sVFSDirEntry) + strlen(n->name);
		n = n->next;
	}

	/* the root-directory is distributed on the fs-driver and the kernel */
	/* therefore we have to read it from the fs-driver, too */
	/* but don't do that if we're the kernel (vfsr does not work then) */
	if(node->parent == NULL && pid != KERNEL_PID) {
		const u32 bufSize = 1024;
		u32 c,curSize = bufSize;
		fsBytes = (u8*)kheap_alloc(bufSize);
		if(fsBytes != NULL) {
			tFileNo rfile = vfs_real_openPath(pid,VFS_READ,"/");
			if(rfile >= 0) {
				while((c = vfs_readFile(pid,rfile,fsBytes + fsByteCount,bufSize)) > 0) {
					fsByteCount += c;
					if(c < bufSize)
						break;

					curSize += bufSize;
					fsBytesDup = kheap_realloc(fsBytes,curSize);
					if(fsBytesDup == NULL) {
						byteCount = 0;
						break;
					}
					fsBytes = fsBytesDup;
				}
				vfs_closeFile(pid,rfile);
			}
		}
		byteCount += fsByteCount;
	}

	if(byteCount > 0) {
		/* now allocate mem on the heap and copy all data into it */
		fsBytesDup = kheap_realloc(fsBytes,byteCount);
		if(fsBytesDup == NULL)
			byteCount = 0;
		else {
			u16 len;
			sVFSDirEntry *dirEntry = (sVFSDirEntry*)(fsBytesDup + fsByteCount);
			fsBytes = fsBytesDup;
			n = vfs_node_getFirstChild(node);
			while(n != NULL) {
				if(node->parent == NULL && (strcmp(n->name,".") == 0 || strcmp(n->name,"..") == 0)) {
					n = n->next;
					continue;
				}
				len = strlen(n->name);
				dirEntry->nodeNo = vfs_node_getNo(n);
				dirEntry->nameLen = len;
				dirEntry->recLen = sizeof(sVFSDirEntry) + len;
				memcpy(dirEntry + 1,n->name,len);
				dirEntry = (sVFSDirEntry*)((u8*)dirEntry + dirEntry->recLen);
				n = n->next;
			}
		}
	}

	if(offset > byteCount)
		offset = byteCount;
	byteCount = MIN(byteCount - offset,count);
	if(byteCount > 0) {
		/* simply copy the data to the buffer */
		memcpy(buffer,fsBytes + offset,byteCount);
	}
	kheap_free(fsBytes);
	return byteCount;
}
