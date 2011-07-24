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
#include <sys/task/proc.h>
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
	off_t pos;
	void *data;
} sFileContent;

static void vfs_file_destroy(sVFSNode *n);
static off_t vfs_file_seek(pid_t pid,sVFSNode *node,off_t position,off_t offset,uint whence);

sVFSNode *vfs_file_create(pid_t pid,sVFSNode *parent,char *name,fRead read,fWrite write) {
	sVFSNode *node;
	node = vfs_node_create(pid,parent,name);
	if(node == NULL)
		return NULL;

	node->mode = FILE_DEF_MODE;
	node->read = read;
	node->write = write;
	node->seek = vfs_file_seek;
	node->close = NULL;
	node->destroy = vfs_file_destroy;
	node->data = NULL;
	if(read == vfs_file_read) {
		sFileContent *con = (sFileContent*)cache_alloc(sizeof(sFileContent));
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
		cache_free(con->data);
		cache_free(con);
		n->data = NULL;
	}
}

static off_t vfs_file_seek(pid_t pid,sVFSNode *node,off_t position,off_t offset,uint whence) {
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

ssize_t vfs_file_read(pid_t pid,file_t file,sVFSNode *node,USER void *buffer,off_t offset,
		size_t count) {
	UNUSED(pid);
	UNUSED(file);
	size_t byteCount;
	sFileContent *con = (sFileContent*)node->data;
	/* no data available? */
	if(con->data == NULL)
		return 0;

	if(offset > con->pos)
		offset = con->pos;
	byteCount = MIN((size_t)(con->pos - offset),count);
	if(byteCount > 0) {
		/* simply copy the data to the buffer */
		memcpy(buffer,(uint8_t*)con->data + offset,byteCount);
	}
	return byteCount;
}

ssize_t vfs_file_write(pid_t pid,file_t file,sVFSNode *n,USER const void *buffer,off_t offset,
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

		con->data = cache_alloc(newSize);
		/* reset position */
		con->pos = 0;
	}
	/* need to increase cache-size? */
	else if(con->size < offset + count) {
		/* ensure that we allocate enough memory */
		newSize = MAX(offset + count,con->size * 2);
		if(newSize > MAX_VFS_FILE_SIZE)
			return ERR_NOT_ENOUGH_MEM;

		con->data = cache_realloc(con->data,newSize);
	}

	/* all ok? */
	if(con->data != NULL) {
		/* set total size and number of used bytes */
		if(newSize)
			con->size = newSize;
		/* copy the data into the cache; this may segfault, which will leave the the state of the
		 * file as it was before, except that we've increased the buffer-size */
		memcpy((uint8_t*)con->data + offset,buffer,count);
		/* we have checked size for overflow. so it is ok here */
		con->pos = MAX(con->pos,(off_t)(offset + count));
		return count;
	}
	/* don't throw the data away, use the old version */
	con->data = oldData;
	return ERR_NOT_ENOUGH_MEM;
}
