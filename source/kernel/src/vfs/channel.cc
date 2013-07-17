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
#include <sys/mem/sllnodes.h>
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
	Thread *curClient;
	/* a list for sending messages to the device */
	sSLList sendList;
	/* a list for reading messages from the device */
	sSLList recvList;
} sChannel;

typedef struct {
	msgid_t id;
	size_t length;
	Thread *thread;
} sMessage;

static void vfs_chan_destroy(sVFSNode *n);
static off_t vfs_chan_seek(pid_t pid,sVFSNode *node,off_t position,off_t offset,uint whence);
static size_t vfs_chan_getSize(pid_t pid,sVFSNode *node);
static void vfs_chan_close(pid_t pid,sFile *file,sVFSNode *node);
static sMessage *vfs_chan_getMsg(Thread *t,sSLList *list,ushort flags);

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
	node->getSize = vfs_chan_getSize;
	node->close = vfs_chan_close;
	node->destroy = vfs_chan_destroy;
	node->data = NULL;
	chan = (sChannel*)cache_alloc(sizeof(sChannel));
	if(!chan) {
		vfs_node_destroy(node);
		return NULL;
	}
	sll_init(&chan->sendList,slln_allocNode,slln_freeNode);
	sll_init(&chan->recvList,slln_allocNode,slln_freeNode);
	chan->curClient = NULL;
	chan->used = false;
	chan->closed = false;
	node->data = chan;
	/* auto-destroy on the last close() */
	node->refCount--;
	vfs_node_append(parent,node);
	return node;
}

static void vfs_chan_destroy(sVFSNode *n) {
	sChannel *chan = (sChannel*)n->data;
	if(chan) {
		/* we have to reset the last client for the device here */
		if(n->parent)
			vfs_device_clientRemoved(n->parent,n);
		/* clear send and receive list */
		sll_clear(&chan->recvList,true);
		sll_clear(&chan->sendList,true);
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

static size_t vfs_chan_getSize(A_UNUSED pid_t pid,sVFSNode *node) {
	sChannel *chan = (sChannel*)node->data;
	return chan ? sll_length(&chan->sendList) + sll_length(&chan->recvList) : 0;
}

static void vfs_chan_close(pid_t pid,sFile *file,sVFSNode *node) {
	sChannel *chan = (sChannel*)node->data;
	if(node->name == NULL)
		vfs_node_destroy(node);
	else {
		if(vfs_isDevice(file)) {
			chan->used = false;
			chan->curClient = NULL;
		}

		if(node->refCount > 1)
			vfs_node_destroy(node);
		else {
			/* notify the driver, if it is one */
			vfs_devmsgs_close(pid,file,node);

			/* if there are message for the driver we don't want to throw them away */
			/* if there are any in the receivelist (and no references of the node) we
			 * can throw them away because no one will read them anymore
			 * (it means that the client has already closed the file) */
			/* note also that we can assume that the driver is still running since we
			 * would have deleted the whole device-node otherwise */
			if(sll_length(&chan->sendList) == 0)
				vfs_node_destroy(node);
			else
				chan->closed = true;
		}
	}
}

void vfs_chan_setUsed(sVFSNode *node,bool used) {
	sChannel *chan = (sChannel*)node->data;
	if(chan)
		chan->used = used;
}

bool vfs_chan_hasReply(const sVFSNode *node) {
	sChannel *chan = (sChannel*)node->data;
	return chan && sll_length(&chan->recvList) > 0;
}

bool vfs_chan_hasWork(const sVFSNode *node) {
	sChannel *chan = (sChannel*)node->data;
	return chan && !chan->used && sll_length(&chan->sendList) > 0;
}

ssize_t vfs_chan_send(A_UNUSED pid_t pid,ushort flags,sVFSNode *n,msgid_t id,
		USER const void *data1,size_t size1,USER const void *data2,size_t size2) {
	sSLList *list;
	Thread *t = Thread::getRunning();
	sChannel *chan = (sChannel*)n->data;
	sMessage *msg1,*msg2 = NULL;
	Thread *recipient = t;

#if PRINT_MSGS
	sProc *p = proc_getByPid(pid);
	vid_printf("%d:%d:%s -> %d (%d b) %s:%x (%s)\n",
			t->tid,pid,p ? p->command : "??",id,size1,n->name,n,n->parent->name);
	if(data2) {
		vid_printf("%d:%d:%s -> %d (%d b) %s:%x (%s)\n",
				t->tid,pid,p ? p->command : "??",id,size2,n->name,n,n->parent->name);
	}
#endif

	/* devices write to the receive-list (which will be read by other processes) */
	if(flags & VFS_DEVICE) {
		assert(data2 == NULL && size2 == 0);
		list = &chan->recvList;
	}
	/* other processes write to the send-list (which will be read by the driver) */
	else
		list = &chan->sendList;

	/* create message and copy data to it */
	msg1 = (sMessage*)cache_alloc(sizeof(sMessage) + size1);
	if(msg1 == NULL)
		return -ENOMEM;

	msg1->length = size1;
	msg1->id = id;
	if(data1) {
		Thread::addHeapAlloc(msg1);
		memcpy(msg1 + 1,data1,size1);
		Thread::remHeapAlloc(msg1);
	}

	if(data2) {
		msg2 = (sMessage*)cache_alloc(sizeof(sMessage) + size2);
		if(msg2 == NULL)
			return -ENOMEM;

		msg2->length = size2;
		msg2->id = id;
		Thread::addHeapAlloc(msg1);
		Thread::addHeapAlloc(msg2);
		memcpy(msg2 + 1,data2,size2);
		Thread::remHeapAlloc(msg2);
		Thread::remHeapAlloc(msg1);
	}

	spinlock_aquire(&n->lock);
	if(n->name == NULL) {
		spinlock_release(&n->lock);
		return -EDESTROYED;
	}

	/* note that we do that here, because memcpy can fail because the page is swapped out for
	 * example. we can't hold the lock during that operation */
	spinlock_aquire(&waitLock);

	/* set recipient */
	if(flags & VFS_DEVICE)
		recipient = id >= MSG_DEF_RESPONSE ? chan->curClient : NULL;
	msg1->thread = recipient;
	if(data2)
		msg2->thread = recipient;

	/* append to list */
	if(!sll_append(list,msg1))
		goto error;
	if(msg2 && !sll_append(list,msg2))
		goto errorRem;

	/* notify the driver */
	if(list == &chan->sendList) {
		vfs_device_addMsg(n->parent);
		if(msg2)
			vfs_device_addMsg(n->parent);
		ev_wakeup(EVI_CLIENT,(evobj_t)n->parent);
	}
	else {
		/* notify other possible waiters */
		if(recipient)
			ev_unblock(recipient);
		else
			ev_wakeup(EVI_RECEIVED_MSG,(evobj_t)n);
	}
	spinlock_release(&waitLock);
	spinlock_release(&n->lock);
	return 0;

errorRem:
	sll_removeFirstWith(list,msg1);
error:
	spinlock_release(&waitLock);
	spinlock_release(&n->lock);
	cache_free(msg1);
	cache_free(msg2);
	return -ENOMEM;
}

ssize_t vfs_chan_receive(A_UNUSED pid_t pid,ushort flags,sVFSNode *node,USER msgid_t *id,
		USER void *data,size_t size,bool block,bool ignoreSigs) {
	sSLList *list;
	Thread *t = Thread::getRunning();
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
	if(flags & VFS_DEVICE) {
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
	while((msg = vfs_chan_getMsg(t,list,flags)) == NULL) {
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
			Thread::switchNoSigs();
		else
			Thread::switchAway();

		if(sig_hasSignalFor(t->tid))
			return -EINTR;
		/* if we waked up and there is no message, the driver probably died */
		spinlock_aquire(&node->lock);
		if(node->name == NULL) {
			spinlock_release(&node->lock);
			return -EDESTROYED;
		}
	}

	if(event == EVI_CLIENT) {
		vfs_device_remMsg(node->parent);
		chan->curClient = msg->thread;
	}
	spinlock_release(&node->lock);
	if(data && msg->length > size) {
		cache_free(msg);
		return -EINVAL;
	}

#if PRINT_MSGS
	sProc *p = proc_getByPid(pid);
	vid_printf("%d:%d:%s <- %d (%d b) %s:%x (%s)\n",
			t->tid,pid,p ? p->command : "??",msg->id,msg->length,node->name,node,node->parent->name);
#endif

	/* copy data and id; since it may fail we have to ensure that our resources are free'd */
	Thread::addHeapAlloc(msg);
	if(data)
		memcpy(data,msg + 1,msg->length);
	if(id)
		*id = msg->id;
	Thread::remHeapAlloc(msg);

	res = msg->length;
	cache_free(msg);
	return res;
}

static sMessage *vfs_chan_getMsg(Thread *t,sSLList *list,ushort flags) {
	sSLNode *n,*p;
	/* drivers get always the first message */
	if(flags & VFS_DEVICE)
		return (sMessage*)sll_removeFirst(list);

	/* find the message for this client-thread */
	p = NULL;
	for(n = sll_begin(list); n != NULL; p = n, n = n->next) {
		sMessage *msg = (sMessage*)n->data;
		if(msg->thread == NULL || msg->thread == t) {
			sll_removeNode(list,n,p);
			return msg;
		}
	}
	return NULL;
}

void vfs_chan_print(const sVFSNode *n) {
	size_t i;
	sChannel *chan = (sChannel*)n->data;
	sSLList *lists[] = {NULL,NULL};
	lists[0] = &chan->sendList;
	lists[1] = &chan->recvList;
	for(i = 0; i < ARRAY_SIZE(lists); i++) {
		size_t j,count = sll_length(lists[i]);
		vid_printf("Channel %s %s: (%zu,%s)\n",n->name,i ? "recvs" : "sends",count,
		                                                     chan->used ? "used" : "-");
		for(j = 0; j < count; j++) {
			sMessage *msg = (sMessage*)sll_get(lists[i],j);
			vid_printf("\tid=%u len=%zu, thread=%d:%d:%s\n",msg->id,msg->length,
					msg->thread ? msg->thread->tid : -1,
					msg->thread ? msg->thread->proc->pid : -1,
					msg->thread ? msg->thread->proc->command : "");
		}
	}
}
