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
#include <sys/mem/cache.h>
#include <sys/mem/sllnodes.h>
#include <sys/mem/vmm.h>
#include <sys/task/thread.h>
#include <sys/task/signals.h>
#include <sys/task/event.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/pipe.h>
#include <sys/spinlock.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

typedef struct {
	uint8_t noReader;
	/* total number of bytes available */
	size_t total;
	/* a list with sPipeData */
	sSLList list;
} sPipe;

typedef struct {
	size_t length;
	off_t offset;
	uint8_t data[];
} sPipeData;

static size_t vfs_pipe_getSize(pid_t pid,sVFSNode *node);
static void vfs_pipe_destroy(sVFSNode *n);
static void vfs_pipe_close(pid_t pid,sFile *file,sVFSNode *node);
static ssize_t vfs_pipe_read(tid_t pid,sFile *file,sVFSNode *node,void *buffer,off_t offset,
		size_t count);
static ssize_t vfs_pipe_write(pid_t pid,sFile *file,sVFSNode *node,const void *buffer,
		off_t offset,size_t count);

sVFSNode *vfs_pipe_create(pid_t pid,sVFSNode *parent) {
	sPipe *pipe;
	sVFSNode *node;
	char *name = vfs_node_getId(pid);
	if(!name)
		return NULL;
	node = vfs_node_create(pid,name);
	if(node == NULL) {
		cache_free(name);
		return NULL;
	}

	node->mode = MODE_TYPE_PIPE | S_IRUSR | S_IWUSR;
	node->read = vfs_pipe_read;
	node->write = vfs_pipe_write;
	node->seek = NULL;
	node->getSize = vfs_pipe_getSize;
	node->destroy = vfs_pipe_destroy;
	node->close = vfs_pipe_close;
	node->data = NULL;
	pipe = (sPipe*)cache_alloc(sizeof(sPipe));
	if(!pipe) {
		vfs_node_destroy(node);
		return NULL;
	}
	pipe->noReader = false;
	pipe->total = 0;
	sll_init(&pipe->list,slln_allocNode,slln_freeNode);
	node->data = pipe;
	/* auto-destroy on the last close() */
	node->refCount--;
	vfs_node_append(parent,node);
	return node;
}

static size_t vfs_pipe_getSize(A_UNUSED pid_t pid,sVFSNode *node) {
	sPipe *pipe = (sPipe*)node->data;
	return pipe ? pipe->total : 0;
}

static void vfs_pipe_destroy(sVFSNode *n) {
	sPipe *pipe = (sPipe*)n->data;
	if(pipe) {
		sll_clear(&pipe->list,true);
		cache_free(pipe);
		n->data = NULL;
	}
}

static void vfs_pipe_close(pid_t pid,sFile *file,sVFSNode *node) {
	/* if there are still more than 1 user, notify the other */
	if(node->name != NULL && node->refCount > 1) {
		/* if thats the read-end, save that there is no reader anymore and wakeup the writers */
		if(vfs_fcntl(pid,file,F_GETACCESS,0) == VFS_READ) {
			sPipe *pipe = (sPipe*)node->data;
			pipe->noReader = true;
			ev_wakeup(EVI_PIPE_EMPTY,(evobj_t)node);
		}
		/* otherwise write EOF in the pipe */
		else
			vfs_writeFile(pid,file,NULL,0);
	}
	/* in any case, destroy the node, i.e. decrease references */
	vfs_node_destroy(node);
}

static ssize_t vfs_pipe_read(A_UNUSED tid_t pid,A_UNUSED sFile *file,sVFSNode *node,
		USER void *buffer,off_t offset,size_t count) {
	size_t byteCount,total;
	sThread *t = thread_getRunning();
	sPipe *pipe = (sPipe*)node->data;
	sPipeData *data;

	/* wait until data is available */
	spinlock_aquire(&node->lock);
	if(node->name == NULL) {
		spinlock_release(&node->lock);
		return -EDESTROYED;
	}
	while(sll_length(&pipe->list) == 0) {
		ev_wait(t,EVI_PIPE_FULL,(evobj_t)node);
		spinlock_release(&node->lock);

		thread_switch();

		if(sig_hasSignalFor(t->tid))
			return -EINTR;
		spinlock_aquire(&node->lock);
		if(node->name == NULL) {
			spinlock_release(&node->lock);
			return -EDESTROYED;
		}
	}

	data = (sPipeData*)sll_get(&pipe->list,0);
	/* empty message indicates EOF */
	if(data->length == 0) {
		spinlock_release(&node->lock);
		return 0;
	}

	total = 0;
	while(true) {
		/* copy */
		vassert(offset >= data->offset,"Illegal offset");
		vassert((off_t)data->length >= (offset - data->offset),"Illegal offset");
		byteCount = MIN(data->length - (offset - data->offset),count);
		thread_addLock(&node->lock);
		memcpy((uint8_t*)buffer + total,data->data + (offset - data->offset),byteCount);
		thread_remLock(&node->lock);
		/* remove if read completely */
		if(byteCount + (offset - data->offset) == data->length) {
			cache_free(data);
			sll_removeFirst(&pipe->list);
		}
		count -= byteCount;
		total += byteCount;
		pipe->total -= byteCount;
		offset += byteCount;
		if(count == 0)
			break;
		/* wait until data is available */
		while(sll_length(&pipe->list) == 0) {
			/* before we go to sleep we have to notify others that we've read data. otherwise
			 * we may cause a deadlock here */
			ev_wakeup(EVI_PIPE_EMPTY,(evobj_t)node);
			ev_wait(t,EVI_PIPE_FULL,(evobj_t)node);
			spinlock_release(&node->lock);

			/* TODO we can't accept signals here, right? since we've already read something, which
			 * we have to deliver to the user. the only way I can imagine would be to put it back.. */
			thread_switchNoSigs();

			spinlock_aquire(&node->lock);
			if(node->name == NULL) {
				spinlock_release(&node->lock);
				return -EDESTROYED;
			}
		}
		data = (sPipeData*)sll_get(&pipe->list,0);
		/* keep the empty one for the next transfer */
		if(data->length == 0)
			break;
	}
	spinlock_release(&node->lock);
	/* wakeup all threads that wait for writing in this node */
	ev_wakeup(EVI_PIPE_EMPTY,(evobj_t)node);
	return total;
}

static ssize_t vfs_pipe_write(A_UNUSED pid_t pid,A_UNUSED sFile *file,sVFSNode *node,
		USER const void *buffer,off_t offset,size_t count) {
	sPipeData *data;
	sThread *t = thread_getRunning();
	sPipe *pipe = (sPipe*)node->data;

	/* Note that the size-check doesn't ensure that the total pipe-size can't be larger than the
	 * maximum. Its not really critical here, I think. Therefore its enough for making sure that
	 * we don't write all the time without reading most of the data. */

	/* wait while our node is full */
	if(count) {
		spinlock_aquire(&node->lock);
		if(node->name == NULL) {
			spinlock_release(&node->lock);
			return -EDESTROYED;
		}
		while((pipe->total + count) >= MAX_VFS_FILE_SIZE) {
			ev_wait(t,EVI_PIPE_EMPTY,(evobj_t)node);
			spinlock_release(&node->lock);

			thread_switchNoSigs();

			/* if we wake up and there is no pipe-reader anymore, send a signal to us so that we
			 * either terminate or react on that signal. */
			spinlock_aquire(&node->lock);
			if(node->name == NULL) {
				spinlock_release(&node->lock);
				return -EDESTROYED;
			}
			if(pipe->noReader) {
				spinlock_release(&node->lock);
				proc_addSignalFor(pid,SIG_PIPE_CLOSED);
				return -EPIPE;
			}
		}
		spinlock_release(&node->lock);
	}

	/* build pipe-data */
	data = (sPipeData*)cache_alloc(sizeof(sPipeData) + count);
	if(data == NULL)
		return -ENOMEM;
	data->offset = offset;
	data->length = count;
	if(count) {
		thread_addHeapAlloc(data);
		memcpy(data->data,buffer,count);
		thread_remHeapAlloc(data);
	}

	/* append */
	spinlock_aquire(&node->lock);
	if(node->name == NULL) {
		spinlock_release(&node->lock);
		cache_free(data);
		return -EDESTROYED;
	}
	if(!sll_append(&pipe->list,data)) {
		spinlock_release(&node->lock);
		cache_free(data);
		return -ENOMEM;
	}
	pipe->total += count;
	spinlock_release(&node->lock);
	ev_wakeup(EVI_PIPE_FULL,(evobj_t)node);
	return count;
}
