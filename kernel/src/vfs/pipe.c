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
#include <sys/mem/kheap.h>
#include <sys/task/thread.h>
#include <sys/task/signals.h>
#include <sys/task/event.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/pipe.h>
#include <string.h>
#include <errors.h>

typedef struct {
	u8 noReader;
	/* total number of bytes available */
	u32 total;
	/* a list with sPipeData */
	sSLList *list;
} sPipe;

typedef struct {
	u32 length;
	u32 offset;
	u8 data[];
} sPipeData;

static void vfs_pipe_destroy(sVFSNode *n);
static void vfs_pipe_close(tPid pid,tFileNo file,sVFSNode *node);
static s32 vfs_pipe_read(tTid pid,tFileNo file,sVFSNode *node,void *buffer,u32 offset,u32 count);
static s32 vfs_pipe_write(tPid pid,tFileNo file,sVFSNode *node,const void *buffer,
		u32 offset,u32 count);

sVFSNode *vfs_pipe_create(tPid pid,sVFSNode *parent) {
	sPipe *pipe;
	sVFSNode *node;
	char *name = vfs_node_getId(pid);
	if(!name)
		return NULL;
	node = vfs_node_create(parent,name);
	if(node == NULL) {
		kheap_free(name);
		return NULL;
	}

	node->owner = pid;
	node->mode = MODE_TYPE_PIPE | MODE_OWNER_READ | MODE_OWNER_WRITE |
			MODE_OTHER_READ | MODE_OTHER_WRITE;
	node->read = vfs_pipe_read;
	node->write = vfs_pipe_write;
	node->seek = NULL;
	node->destroy = vfs_pipe_destroy;
	node->close = vfs_pipe_close;
	node->data = NULL;
	pipe = (sPipe*)kheap_alloc(sizeof(sPipe));
	if(!pipe) {
		vfs_node_destroy(node);
		return NULL;
	}
	pipe->noReader = false;
	pipe->total = 0;
	pipe->list = NULL;
	node->data = pipe;
	return node;
}

static void vfs_pipe_destroy(sVFSNode *n) {
	sPipe *pipe = (sPipe*)n->data;
	if(pipe) {
		sll_destroy(pipe->list,true);
		pipe->list = NULL;
	}
}

static void vfs_pipe_close(tPid pid,tFileNo file,sVFSNode *node) {
	/* last usage? */
	if(node->refCount == 0) {
		/* remove pipe-nodes if there are no references anymore */
		vfs_node_destroy(node);
	}
	/* there are still references to the pipe, write an EOF into it (0 references
	 * mean that the pipe should be removed) */
	else {
		/* if thats the read-end, save that there is no reader anymore and wakeup the writers */
		if(vfs_fcntl(pid,file,F_GETACCESS,0) == VFS_READ) {
			sPipe *pipe = (sPipe*)node->data;
			pipe->noReader = true;
			ev_wakeup(EVI_PIPE_EMPTY,(tEvObj)node);
		}
		/* otherwise write EOF in the pipe */
		else
			vfs_writeFile(pid,file,NULL,0);
	}
}

static s32 vfs_pipe_read(tTid pid,tFileNo file,sVFSNode *node,void *buffer,u32 offset,u32 count) {
	UNUSED(pid);
	UNUSED(file);
	s32 byteCount,total;
	sThread *t = thread_getRunning();
	sPipe *pipe = (sPipe*)node->data;
	volatile sPipe *vpipe = pipe;
	sPipeData *data;

	/* wait until data is available */
	/* don't cache the list here, because the pointer changes if the list is NULL */
	while(sll_length(vpipe->list) == 0) {
		ev_wait(t->tid,EVI_PIPE_FULL,(tEvObj)node);
		thread_switch();
		if(sig_hasSignalFor(t->tid))
			return ERR_INTERRUPTED;
	}

	data = sll_get(vpipe->list,0);
	/* empty message indicates EOF */
	if(data->length == 0)
		return 0;

	total = 0;
	while(1) {
		/* copy */
		vassert(offset >= data->offset,"Illegal offset");
		vassert(data->length >= (offset - data->offset),"Illegal offset");
		byteCount = MIN(data->length - (offset - data->offset),count);
		memcpy((u8*)buffer + total,data->data + (offset - data->offset),byteCount);
		count -= byteCount;
		total += byteCount;
		/* remove if read completely */
		if(byteCount + (offset - data->offset) == data->length) {
			kheap_free(data);
			sll_removeIndex(vpipe->list,0);
		}
		pipe->total -= byteCount;
		offset += byteCount;
		if(count == 0)
			break;
		/* wait until data is available */
		while(sll_length(vpipe->list) == 0) {
			/* before we go to sleep we have to notify others that we've read data. otherwise
			 * we may cause a deadlock here */
			ev_wakeup(EVI_PIPE_EMPTY,(tEvObj)node);
			ev_wait(t->tid,EVI_PIPE_FULL,(tEvObj)node);
			/* TODO we can't accept signals here, right? since we've already read something, which
			 * we have to deliver to the user. the only way I can imagine would be to put it back..
			 */
			thread_switchNoSigs();
		}
		data = sll_get(vpipe->list,0);
		/* keep the empty one for the next transfer */
		if(data->length == 0)
			break;
	}
	/* wakeup all threads that wait for writing in this node */
	ev_wakeup(EVI_PIPE_EMPTY,(tEvObj)node);
	return total;
}

static s32 vfs_pipe_write(tPid pid,tFileNo file,sVFSNode *node,const void *buffer,
		u32 offset,u32 count) {
	UNUSED(pid);
	UNUSED(file);
	sPipeData *data;
	sThread *t = thread_getRunning();
	sPipe *pipe = (sPipe*)node->data;
	volatile sPipe *vpipe = pipe;

	/* wait while our node is full */
	if(count) {
		while((vpipe->total + count) >= MAX_VFS_FILE_SIZE) {
			ev_wait(t->tid,EVI_PIPE_EMPTY,(tEvObj)node);
			thread_switchNoSigs();
			/* if we wake up and there is no pipe-reader anymore, send a signal to us so that we
			 * either terminate or react on that signal. */
			if(vpipe->noReader) {
				sig_addSignalFor(pid,SIG_PIPE_CLOSED);
				return ERR_EOF;
			}
		}
	}

	/* build pipe-data */
	data = (sPipeData*)kheap_alloc(sizeof(sPipeData) + count);
	if(data == NULL)
		return ERR_NOT_ENOUGH_MEM;
	data->offset = offset;
	data->length = count;
	if(count)
		memcpy(data->data,buffer,count);

	/* create list, if necessary */
	if(pipe->list == NULL) {
		pipe->list = sll_create();
		if(pipe->list == NULL) {
			kheap_free(data);
			return ERR_NOT_ENOUGH_MEM;
		}
	}

	/* append */
	if(!sll_append(pipe->list,data)) {
		kheap_free(data);
		return ERR_NOT_ENOUGH_MEM;
	}
	pipe->total += count;
	ev_wakeup(EVI_PIPE_FULL,(tEvObj)node);
	return count;
}
