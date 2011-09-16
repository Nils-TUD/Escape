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
#include <sys/vfs/server.h>
#include <sys/video.h>
#include <sys/klock.h>
#include <esc/messages.h>
#include <esc/sllist.h>
#include <assert.h>
#include <errors.h>

#define DRV_IMPL(funcs,func)		(((funcs) & (func)) != 0)
#define DRV_IS_FS(funcs)			((funcs) == DRV_FS)

typedef struct {
	/* whether there is data to read or not */
	bool isEmpty;
	/* implemented functions */
	uint funcs;
	/* total number of messages in all channels (for the server, not the clients) */
	ulong msgCount;
	/* the last served client */
	sVFSNode *lastClient;
} sServer;

static void vfs_server_close(pid_t pid,file_t file,sVFSNode *node);
static void vfs_server_destroy(sVFSNode *node);
static void vfs_server_wakeupClients(sVFSNode *node,uint events,bool locked);

sVFSNode *vfs_server_create(pid_t pid,sVFSNode *parent,char *name,uint flags) {
	sServer *srv;
	sVFSNode *node = vfs_node_create(pid,name);
	if(node == NULL)
		return NULL;

	node->mode = MODE_TYPE_DRIVER | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	node->read = NULL;
	node->write = NULL;
	node->seek = NULL;
	node->close = vfs_server_close;
	node->destroy = vfs_server_destroy;
	node->data = NULL;
	srv = (sServer*)cache_alloc(sizeof(sServer));
	if(!srv) {
		vfs_node_destroy(node);
		return NULL;
	}
	srv->funcs = flags;
	srv->isEmpty = true;
	srv->lastClient = NULL;
	srv->msgCount = 0;
	node->data = srv;
	vfs_node_append(parent,node);
	return node;
}

static void vfs_server_close(A_UNUSED pid_t pid,A_UNUSED file_t file,sVFSNode *node) {
	vfs_node_destroy(node);
}

static void vfs_server_destroy(sVFSNode *node) {
	if(node->data) {
		/* wakeup all threads that may be waiting for this node so they can check
		 * whether they are affected by the remove of this driver and perform the corresponding
		 * action */
		vfs_server_wakeupClients(node,EV_RECEIVED_MSG | EV_REQ_REPLY | EV_DATA_READABLE,false);
		cache_free(node->data);
		node->data = NULL;
	}
}

void vfs_server_clientRemoved(sVFSNode *node,const sVFSNode *client) {
	sServer *srv = (sServer*)node->data;
	/* we don't have to lock this, because its only called in vfs_chan_destroy(), which can only
	 * be called when this server-node is locked. i.e. it is not possible during getWork() */
	if(srv->lastClient == client)
		srv->lastClient = NULL;
}

bool vfs_server_accepts(const sVFSNode *node,uint id) {
	sServer *srv = (sServer*)node->data;
	if(DRV_IS_FS(srv->funcs))
		return true;
	return id == MSG_DRV_OPEN_RESP || id == MSG_DRV_READ_RESP || id == MSG_DRV_WRITE_RESP;
}

bool vfs_server_supports(const sVFSNode *node,uint funcs) {
	sServer *srv = (sServer*)node->data;
	return DRV_IMPL(srv->funcs,funcs);
}

bool vfs_server_isReadable(const sVFSNode *node) {
	sServer *srv = (sServer*)node->data;
	return !srv->isEmpty;
}

int vfs_server_setReadable(sVFSNode *node,bool readable) {
	sServer *srv = (sServer*)node->data;
	if(!DRV_IMPL(srv->funcs,DRV_READ))
		return ERR_UNSUPPORTED_OP;
	bool wasEmpty = srv->isEmpty;
	srv->isEmpty = !readable;
	if(wasEmpty && readable)
		vfs_server_wakeupClients(node,EV_RECEIVED_MSG | EV_DATA_READABLE,true);
	return 0;
}

void vfs_server_addMsg(sVFSNode *node) {
	sServer *srv = (sServer*)node->data;
	klock_aquire(&node->lock);
	srv->msgCount++;
	klock_release(&node->lock);
}

void vfs_server_remMsg(sVFSNode *node) {
	sServer *srv = (sServer*)node->data;
	klock_aquire(&node->lock);
	assert(srv->msgCount > 0);
	srv->msgCount--;
	klock_release(&node->lock);
}

bool vfs_server_hasWork(sVFSNode *node) {
	sServer *srv = (sServer*)node->data;
	return srv->msgCount > 0;
}

sVFSNode *vfs_server_getWork(sVFSNode *node,bool *cont,bool *retry) {
	sServer *srv = (sServer*)node->data;
	sVFSNode *n,*last;
	bool isValid;
	/* this is a bit more complicated because we want to do it in a fair way. that means every
	 * process that requests something should be served at some time. therefore we store the last
	 * served client and continue from the next one. */

	/* search for a slot that needs work */
	/* we don't need to lock the server-data here; the node with vfs_node_openDir() is sufficient */
	/* because it can't be called twice because the waitLock in vfs prevents it. and nothing of the
	 * server-data that is used here can be changed during this procedure. */
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
			/* if we have checked all clients in this driver, give the other drivers
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

void vfs_server_print(sVFSNode *n) {
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

static void vfs_server_wakeupClients(sVFSNode *node,uint events,bool locked) {
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
