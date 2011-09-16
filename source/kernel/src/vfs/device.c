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
#include <sys/task/event.h>
#include <sys/task/proc.h>
#include <sys/mem/cache.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/device.h>
#include <sys/video.h>
#include <sys/klock.h>
#include <esc/messages.h>
#include <esc/sllist.h>
#include <assert.h>
#include <errors.h>

#define DRV_IMPL(funcs,func)		(((funcs) & (func)) != 0)
#define DRV_IS_FS(mode)				(((mode) & MODE_TYPE_DEVMASK) == MODE_TYPE_FSDEV)

typedef struct {
	/* whether there is data to read or not */
	bool isEmpty;
	/* implemented functions */
	uint funcs;
	/* total number of messages in all channels (for the device, not the clients) */
	ulong msgCount;
	/* the last served client */
	sVFSNode *lastClient;
} sServer;

static void vfs_device_close(pid_t pid,file_t file,sVFSNode *node);
static void vfs_device_destroy(sVFSNode *node);
static void vfs_device_wakeupClients(sVFSNode *node,uint events,bool locked);

sVFSNode *vfs_device_create(pid_t pid,sVFSNode *parent,char *name,uint type,uint ops) {
	sServer *srv;
	sVFSNode *node = vfs_node_create(pid,name);
	if(node == NULL)
		return NULL;

	node->mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	if(type == DEV_TYPE_BLOCK)
		node->mode |= MODE_TYPE_BLKDEV;
	else if(type == DEV_TYPE_CHAR)
		node->mode |= MODE_TYPE_CHARDEV;
	else if(type == DEV_TYPE_FILE)
		node->mode |= MODE_TYPE_FILEDEV;
	else if(type == DEV_TYPE_SERVICE)
		node->mode |= MODE_TYPE_SERVDEV;
	else {
		assert(type == DEV_TYPE_FS);
		node->mode |= MODE_TYPE_FSDEV;
	}
	node->read = NULL;
	node->write = NULL;
	node->seek = NULL;
	node->close = vfs_device_close;
	node->destroy = vfs_device_destroy;
	node->data = NULL;
	srv = (sServer*)cache_alloc(sizeof(sServer));
	if(!srv) {
		vfs_node_destroy(node);
		return NULL;
	}
	srv->funcs = ops;
	/* block- and file-devices are none-empty by default, because their data is always available */
	srv->isEmpty = type != DEV_TYPE_BLOCK && type != DEV_TYPE_FILE;
	srv->lastClient = NULL;
	srv->msgCount = 0;
	node->data = srv;
	vfs_node_append(parent,node);
	return node;
}

static void vfs_device_close(A_UNUSED pid_t pid,A_UNUSED file_t file,sVFSNode *node) {
	vfs_node_destroy(node);
}

static void vfs_device_destroy(sVFSNode *node) {
	if(node->data) {
		/* wakeup all threads that may be waiting for this node so they can check
		 * whether they are affected by the remove of this device and perform the corresponding
		 * action */
		vfs_device_wakeupClients(node,EV_RECEIVED_MSG | EV_REQ_REPLY | EV_DATA_READABLE,false);
		cache_free(node->data);
		node->data = NULL;
	}
}

void vfs_device_clientRemoved(sVFSNode *node,const sVFSNode *client) {
	sServer *srv = (sServer*)node->data;
	/* we don't have to lock this, because its only called in vfs_chan_destroy(), which can only
	 * be called when this device-node is locked. i.e. it is not possible during getWork() */
	if(srv->lastClient == client)
		srv->lastClient = NULL;
}

bool vfs_device_accepts(const sVFSNode *node,uint id) {
	if(DRV_IS_FS(node->mode))
		return true;
	return id == MSG_DEV_OPEN_RESP || id == MSG_DEV_READ_RESP || id == MSG_DEV_WRITE_RESP;
}

bool vfs_device_supports(const sVFSNode *node,uint funcs) {
	sServer *srv = (sServer*)node->data;
	return DRV_IMPL(srv->funcs,funcs);
}

bool vfs_device_isReadable(const sVFSNode *node) {
	sServer *srv = (sServer*)node->data;
	return !srv->isEmpty;
}

int vfs_device_setReadable(sVFSNode *node,bool readable) {
	sServer *srv = (sServer*)node->data;
	if(!DRV_IMPL(srv->funcs,DEV_READ))
		return ERR_UNSUPPORTED_OP;
	bool wasEmpty = srv->isEmpty;
	srv->isEmpty = !readable;
	if(wasEmpty && readable)
		vfs_device_wakeupClients(node,EV_RECEIVED_MSG | EV_DATA_READABLE,true);
	return 0;
}

void vfs_device_addMsg(sVFSNode *node) {
	sServer *srv = (sServer*)node->data;
	klock_aquire(&node->lock);
	srv->msgCount++;
	klock_release(&node->lock);
}

void vfs_device_remMsg(sVFSNode *node) {
	sServer *srv = (sServer*)node->data;
	klock_aquire(&node->lock);
	assert(srv->msgCount > 0);
	srv->msgCount--;
	klock_release(&node->lock);
}

bool vfs_device_hasWork(sVFSNode *node) {
	sServer *srv = (sServer*)node->data;
	return srv->msgCount > 0;
}

sVFSNode *vfs_device_getWork(sVFSNode *node,bool *cont,bool *retry) {
	sServer *srv = (sServer*)node->data;
	sVFSNode *n,*last;
	bool isValid;
	/* this is a bit more complicated because we want to do it in a fair way. that means every
	 * process that requests something should be served at some time. therefore we store the last
	 * served client and continue from the next one. */

	/* search for a slot that needs work */
	/* we don't need to lock the device-data here; the node with vfs_node_openDir() is sufficient */
	/* because it can't be called twice because the waitLock in vfs prevents it. and nothing of the
	 * device-data that is used here can be changed during this procedure. */
	n = vfs_node_openDir(node,true,&isValid);
	/* if there are no messages at all or the node is invalid, stop right now */
	if(!isValid || srv->msgCount == 0) {
		vfs_node_closeDir(node,true);
		return NULL;
	}

	last = srv->lastClient;
	if(last != NULL) {
		if(last->next == NULL) {
			vfs_node_closeDir(node,true);
			/* if we have checked all clients in this device, give the other devices
			 * a chance (if there are any others) */
			srv->lastClient = NULL;
			*retry = true;
			return NULL;
		}
		else
			n = last->next;
	}

searchBegin:
	while(n != NULL && n != last) {
		/* data available? */
		if(vfs_chan_hasWork(n)) {
			vfs_node_closeDir(node,true);
			*cont = false;
			srv->lastClient = n;
			return n;
		}
		n = n->next;
	}
	vfs_node_closeDir(node,true);
	/* if we have looked through all nodes and the last one has a message again, we have to
	 * store it because we'll not check it again */
	if(last && n == last && vfs_chan_hasWork(n))
		return n;
	if(srv->lastClient) {
		n = vfs_node_openDir(node,true,&isValid);
		if(!isValid) {
			vfs_node_closeDir(node,true);
			return NULL;
		}
		srv->lastClient = NULL;
		goto searchBegin;
	}
	return NULL;
}

void vfs_device_print(sVFSNode *n) {
	bool isValid;
	sServer *srv = (sServer*)n->data;
	sVFSNode *chan = vfs_node_openDir(n,true,&isValid);
	if(isValid) {
		vid_printf("\t%s (%s):\n",n->name,srv->isEmpty ? "empty" : "full");
		while(chan != NULL) {
			vfs_chan_print(chan);
			chan = chan->next;
		}
		vid_printf("\n");
	}
	vfs_node_closeDir(n,true);
}

static void vfs_device_wakeupClients(sVFSNode *node,uint events,bool locked) {
	bool isValid;
	sVFSNode *n = vfs_node_openDir(node,locked,&isValid);
	if(isValid) {
		while(n != NULL) {
			ev_wakeupm(events,(evobj_t)n);
			n = n->next;
		}
	}
	vfs_node_closeDir(node,locked);
}
