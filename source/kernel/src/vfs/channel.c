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
#include <sys/mem/vmm.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/device.h>
#include <sys/vfs/devmsgs.h>
#include <sys/video.h>
#include <sys/spinlock.h>
#include <esc/messages.h>
#include <esc/sllist.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define PRINT_MSGS			0

typedef struct {
	bool used;
	bool closed;
	/* a list for sending messages to the device */
	sSLList *sendList;
	/* a list for reading messages from the device */
	sSLList *recvList;
} sChannel;

typedef struct {
	msgid_t id;
	size_t length;
} sMessage;

static void vfs_chan_destroy(sVFSNode *n);
static off_t vfs_chan_seek(pid_t pid,sVFSNode *node,off_t position,off_t offset,uint whence);
static void vfs_chan_close(pid_t pid,file_t file,sVFSNode *node);

extern klock_t waitLock;

sVFSNode *vfs_chan_create(pid_t pid,sVFSNode *parent) {
	sChannel *chan;
	sVFSNode *node;
	char *name = vfs_node_getId(pid);
	if(!name)
		return NULL;
	node = vfs_node_create(pid,name);
	if(node == NULL) {
		cache_free(name);
		return NULL;
	}

	node->mode = MODE_TYPE_CHANNEL | S_IRUSR | S_IWUSR;
	node->read = (fRead)vfs_devmsgs_read;
	node->write = (fWrite)vfs_devmsgs_write;
	node->seek = vfs_chan_seek;
	node->close = vfs_chan_close;
	node->destroy = vfs_chan_destroy;
	node->data = NULL;
	chan = (sChannel*)cache_alloc(sizeof(sChannel));
	if(!chan) {
		vfs_node_destroy(node);
		return NULL;
	}
	chan->recvList = NULL;
	chan->sendList = NULL;
	chan->used = false;
	chan->closed = false;
	node->data = chan;
	vfs_node_append(parent,node);
	return node;
}

static void vfs_chan_destroy(sVFSNode *n) {
	sChannel *chan = (sChannel*)n->data;
	if(chan) {
		/* we have to reset the last client for the device here */
		vfs_device_clientRemoved(n->parent,n);
		/* free send and receive list */
		if(chan->recvList)
			sll_destroy(chan->recvList,true);
		if(chan->sendList)
			sll_destroy(chan->sendList,true);
		cache_free(chan);
		n->data = NULL;
	}
}

static off_t vfs_chan_seek(A_UNUSED pid_t pid,A_UNUSED sVFSNode *node,off_t position,
		off_t offset,uint whence) {
	switch(whence) {
		case SEEK_SET:
			return offset;
		case SEEK_CUR:
			return position + offset;
		default:
		case SEEK_END:
			/* not supported for devices */
			return -ESPIPE;
	}
}

static void vfs_chan_close(pid_t pid,file_t file,sVFSNode *node) {
	sChannel *chan = (sChannel*)node->data;
	if(node->name == NULL)
		vfs_node_destroy(node);
	else {
		if(vfs_isDevice(file))
			chan->used = false;

		if(node->refCount == 0) {
			/* notify the driver, if it is one */
			vfs_devmsgs_close(pid,file,node);

			/* if there are message for the driver we don't want to throw them away */
			/* if there are any in the receivelist (and no references of the node) we
			 * can throw them away because no one will read them anymore
			 * (it means that the client has already closed the file) */
			/* note also that we can assume that the driver is still running since we
			 * would have deleted the whole device-node otherwise */
			if(sll_length(chan->sendList) == 0)
				vfs_node_destroy(node);
			else
				chan->closed = true;
		}
	}
}

void vfs_chan_setUsed(sVFSNode *node,bool used) {
	sChannel *chan = (sChannel*)node->data;
	chan->used = used;
}

bool vfs_chan_hasReply(const sVFSNode *node) {
	sChannel *chan = (sChannel*)node->data;
	return sll_length(chan->recvList) > 0;
}

bool vfs_chan_hasWork(const sVFSNode *node) {
	sChannel *chan = (sChannel*)node->data;
	return !chan->used && sll_length(chan->sendList) > 0;
}

ssize_t vfs_chan_send(A_UNUSED pid_t pid,file_t file,sVFSNode *n,msgid_t id,
		USER const void *data1,size_t size1,USER const void *data2,size_t size2) {
	sSLList **list;
	sThread *t = thread_getRunning();
	sChannel *chan = (sChannel*)n->data;
	sMessage *msg1,*msg2 = NULL;

#if PRINT_MSGS
	vid_printf("%d:%s sent msg %d with %d bytes over chan %s:%x (device %s)\n",
			pid,proc_getByPid(pid)->command,id,size1,n->name,n,n->parent->name);
	if(data2) {
		vid_printf("%d:%s sent msg %d with %d bytes over chan %s:%x (device %s)\n",
				pid,proc_getByPid(pid)->command,id,size2,n->name,n,n->parent->name);
	}
#endif

	/* devices write to the receive-list (which will be read by other processes) */
	if(vfs_isDevice(file)) {
		assert(data2 == NULL && size2 == 0);
		list = &(chan->recvList);
	}
	/* other processes write to the send-list (which will be read by the driver) */
	else
		list = &(chan->sendList);

	/* create message and copy data to it */
	msg1 = (sMessage*)cache_alloc(sizeof(sMessage) + size1);
	if(msg1 == NULL)
		return -ENOMEM;

	msg1->length = size1;
	msg1->id = id;
	if(data1) {
		thread_addHeapAlloc(t,msg1);
		memcpy(msg1 + 1,data1,size1);
		thread_remHeapAlloc(t,msg1);
	}

	if(data2) {
		msg2 = (sMessage*)cache_alloc(sizeof(sMessage) + size2);
		if(msg2 == NULL)
			return -ENOMEM;

		msg2->length = size2;
		msg2->id = id;
		thread_addHeapAlloc(t,msg1);
		thread_addHeapAlloc(t,msg2);
		memcpy(msg2 + 1,data2,size2);
		thread_remHeapAlloc(t,msg2);
		thread_remHeapAlloc(t,msg1);
	}

	spinlock_aquire(&n->lock);
	if(n->name == NULL) {
		spinlock_release(&n->lock);
		return -EDESTROYED;
	}

	/* note that we do that here, because memcpy can fail because the page is swapped out for
	 * example. we can't hold the lock during that operation */
	spinlock_aquire(&waitLock);
	if(*list == NULL)
		*list = sll_create();

	/* append to list */
	if(!sll_append(*list,msg1))
		goto error;
	if(msg2 && !sll_append(*list,msg2))
		goto errorRem;

	/* notify the driver */
	if(list == &(chan->sendList)) {
		vfs_device_addMsg(n->parent);
		if(msg2)
			vfs_device_addMsg(n->parent);
		ev_wakeup(EVI_CLIENT,(evobj_t)n->parent);
	}
	/* notify all threads that wait on this node for a msg */
	else
		ev_wakeup(EVI_RECEIVED_MSG,(evobj_t)n);
	spinlock_release(&waitLock);
	spinlock_release(&n->lock);
	return 0;

errorRem:
	sll_removeFirstWith(*list,msg1);
error:
	spinlock_release(&waitLock);
	spinlock_release(&n->lock);
	cache_free(msg1);
	cache_free(msg2);
	return -ENOMEM;
}

ssize_t vfs_chan_receive(A_UNUSED pid_t pid,file_t file,sVFSNode *node,USER msgid_t *id,
		USER void *data,size_t size,bool block,bool ignoreSigs) {
	sSLList **list;
	sThread *t = thread_getRunning();
	sChannel *chan = (sChannel*)node->data;
	sVFSNode *waitNode;
	sMessage *msg;
	size_t event;
	ssize_t res;

	spinlock_aquire(&node->lock);
	/* node destroyed? */
	if(node->name == NULL) {
		spinlock_release(&node->lock);
		return -EDESTROYED;
	}

	/* determine list and event to use */
	if(vfs_isDevice(file)) {
		event = EVI_CLIENT;
		list = &chan->sendList;
		waitNode = node->parent;
	}
	else {
		event = EVI_RECEIVED_MSG;
		list = &chan->recvList;
		waitNode = node;
	}

	/* wait until a message arrives */
	while(sll_length(*list) == 0) {
		if(!block) {
			spinlock_release(&node->lock);
			return -EWOULDBLOCK;
		}
		/* if the channel has already been closed, there is no hope of success here */
		if(chan->closed) {
			spinlock_release(&node->lock);
			return -EDESTROYED;
		}
		ev_wait(t,event,(evobj_t)waitNode);
		spinlock_release(&node->lock);

		if(ignoreSigs)
			thread_switchNoSigs();
		else
			thread_switch();

		if(sig_hasSignalFor(t->tid))
			return -EINTR;
		/* if we waked up and there is no message, the driver probably died */
		spinlock_aquire(&node->lock);
		if(node->name == NULL) {
			spinlock_release(&node->lock);
			return -EDESTROYED;
		}
	}

	/* get first element and copy data to buffer */
	msg = (sMessage*)sll_removeFirst(*list);
	if(event == EVI_CLIENT)
		vfs_device_remMsg(node->parent);
	spinlock_release(&node->lock);
	if(data && msg->length > size) {
		cache_free(msg);
		return -EINVAL;
	}

#if PRINT_MSGS
	vid_printf("%d:%s received msg %d from chan %s:%x\n",
			pid,proc_getByPid(pid)->command,msg->id,node->name,node);
#endif

	/* copy data and id; since it may fail we have to ensure that our resources are free'd */
	thread_addHeapAlloc(t,msg);
	if(data)
		memcpy(data,msg + 1,msg->length);
	if(id)
		*id = msg->id;
	thread_remHeapAlloc(t,msg);

	res = msg->length;
	cache_free(msg);
	return res;
}

void vfs_chan_print(const sVFSNode *n) {
	size_t i;
	sChannel *chan = (sChannel*)n->data;
	sSLList *lists[] = {NULL,NULL};
	lists[0] = chan->sendList;
	lists[1] = chan->recvList;
	for(i = 0; i < ARRAY_SIZE(lists); i++) {
		size_t j,count = sll_length(lists[i]);
		vid_printf("\t\tChannel %s %s: (%zu)\n",n->name,i ? "recvs" : "sends",count);
		for(j = 0; j < count; j++) {
			sMessage *msg = (sMessage*)sll_get(lists[i],j);
			vid_printf("\t\t\tid=%u len=%zu\n",msg->id,msg->length);
		}
	}
}
