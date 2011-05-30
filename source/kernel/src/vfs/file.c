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
#include <sys/vfs/file.h>
#include <sys/vfs/node.h>
#include <string.h>
#include <errors.h>

/* the initial size of the write-cache for driver-usage-nodes */
#define VFS_INITIAL_WRITECACHE		128

typedef struct {
	/* size of the buffer */
	size_t size;
	/* currently used size */
	size_t pos;
	void *data;
} sFileContent;

static void vfs_file_destroy(sVFSNode *n);
static int vfs_file_seek(tPid pid,sVFSNode *node,uint position,int offset,uint whence);

sVFSNode *vfs_file_create(tPid pid,sVFSNode *parent,char *name,fRead read,fWrite write) {
	sVFSNode *node;
	node = vfs_node_create(parent,name);
	if(node == NULL)
		return NULL;

	node->owner = pid;
	/* TODO we need write-permissions for other because we have no real user-/group-based
	 * permission-system */
	node->mode = MODE_TYPE_FILE | MODE_OWNER_READ | MODE_OWNER_WRITE | MODE_OTHER_READ
		| MODE_OTHER_WRITE;
	node->read = read;
	node->write = write;
	node->seek = vfs_file_seek;
	node->close = NULL;
	node->destroy = vfs_file_destroy;
	node->data = NULL;
	if(read == vfs_file_read) {
		sFileContent *con = (sFileContent*)kheap_alloc(sizeof(sFileContent));
		if(!con) {
			vfs_node_destroy(node);
			return NULL;
		}
		con->data = NULL;
		con->size = 0;
		con->pos = 0;
		node->data = con;
	}
	return node;
}

static void vfs_file_destroy(sVFSNode *n) {
	sFileContent *con = (sFileContent*)n->data;
	if(con) {
		kheap_free(con);
		n->data = NULL;
	}
}

static int vfs_file_seek(tPid pid,sVFSNode *node,uint position,int offset,uint whence) {
	UNUSED(pid);
	switch(whence) {
		case SEEK_SET:
			return offset;
		case SEEK_CUR:
			/* we can't validate the position here because the content of virtual files may be
			 * generated on demand */
			return position + offset;
		default:
		case SEEK_END: {
			sFileContent *con = (sFileContent*)node->data;
			if(con)
				return con->pos;
			return 0;
		}
	}
}

ssize_t vfs_file_read(tPid pid,tFileNo file,sVFSNode *node,void *buffer,uint offset,size_t count) {
	UNUSED(pid);
	UNUSED(file);
	size_t byteCount;
	sFileContent *con = (sFileContent*)node->data;
	/* no data available? */
	if(con->data == NULL)
		return 0;

	if(offset > con->pos)
		offset = con->pos;
	byteCount = MIN(con->pos - offset,count);
	if(byteCount > 0) {
		/* simply copy the data to the buffer */
		memcpy(buffer,(uint8_t*)con->data + offset,byteCount);
	}
	return byteCount;
}

ssize_t vfs_file_write(tPid pid,tFileNo file,sVFSNode *n,const void *buffer,uint offset,
		size_t count) {
	UNUSED(pid);
	UNUSED(file);
	void *oldData;
	size_t newSize = 0;
	sFileContent *con = (sFileContent*)n->data;
	oldData = con->data;

	/* need to create cache? */
	if(con->data == NULL) {
		newSize = MAX(count,VFS_INITIAL_WRITECACHE);
		/* check for overflow */
		if(newSize > MAX_VFS_FILE_SIZE)
			return ERR_NOT_ENOUGH_MEM;

		con->data = kheap_alloc(newSize);
		/* reset position */
		con->pos = 0;
	}
	/* need to increase cache-size? */
	else if(con->size < offset + count) {
		/* ensure that we allocate enough memory */
		newSize = MAX(offset + count,con->size * 2);
		if(newSize > MAX_VFS_FILE_SIZE)
			return ERR_NOT_ENOUGH_MEM;

		con->data = kheap_realloc(con->data,newSize);
	}

	/* all ok? */
	if(con->data != NULL) {
		/* copy the data into the cache */
		memcpy((uint8_t*)con->data + offset,buffer,count);
		/* set total size and number of used bytes */
		if(newSize)
			con->size = newSize;
		/* we have checked size for overflow. so it is ok here */
		con->pos = MAX(con->pos,offset + count);

		return count;
	}
	/* don't throw the data away, use the old version */
	con->data = oldData;
	return ERR_NOT_ENOUGH_MEM;
}
