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
#include <sys/task/event.h>
#include <sys/mem/kheap.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/server.h>
#include <sys/video.h>
#include <esc/messages.h>
#include <esc/sllist.h>
#include <errors.h>

#define DRV_IMPL(funcs,func)		(((funcs) & (func)) != 0)
#define DRV_IS_FS(funcs)			((funcs) == DRV_FS)

typedef struct {
	/* whether there is data to read or not */
	bool isEmpty;
	/* implemented functions */
	uint funcs;
	/* the last served client */
	sVFSNode *lastClient;
} sServer;

static void vfs_server_close(tPid pid,tFileNo file,sVFSNode *node);
static void vfs_server_destroy(sVFSNode *node);
static void vfs_server_wakeupClients(const sVFSNode *node,uint events);

sVFSNode *vfs_server_create(tPid pid,sVFSNode *parent,char *name,uint flags) {
	sServer *srv;
	sVFSNode *node = vfs_node_create(parent,name);
	if(node == NULL)
		return NULL;

	node->owner = pid;
	node->mode = MODE_TYPE_DRIVER | MODE_OWNER_READ | MODE_OTHER_READ;
	node->read = NULL;
	node->write = NULL;
	node->seek = NULL;
	node->close = vfs_server_close;
	node->destroy = vfs_server_destroy;
	node->data = NULL;
	srv = (sServer*)kheap_alloc(sizeof(sServer));
	if(!srv) {
		vfs_node_destroy(node);
		return NULL;
	}
	srv->funcs = flags;
	srv->isEmpty = true;
	srv->lastClient = NULL;
	node->data = srv;
	return node;
}

static void vfs_server_close(tPid pid,tFileNo file,sVFSNode *node) {
	UNUSED(pid);
	UNUSED(file);
	vfs_node_destroy(node);
}

static void vfs_server_destroy(sVFSNode *node) {
	if(node->data) {
		/* wakeup all threads that may be waiting for this node so they can check
		 * whether they are affected by the remove of this driver and perform the corresponding
		 * action */
		vfs_server_wakeupClients(node,EV_RECEIVED_MSG | EV_REQ_REPLY | EV_DATA_READABLE);
		kheap_free(node->data);
		node->data = NULL;
	}
}

void vfs_server_clientRemoved(sVFSNode *node,const sVFSNode *client) {
	sServer *srv = (sServer*)node->data;
	if(srv->lastClient == client)
		srv->lastClient = NULL;
}

bool vfs_server_isterm(const sVFSNode *node) {
	sServer *srv = (sServer*)node->data;
	return srv->funcs & DRV_TERM;
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
		vfs_server_wakeupClients(node,EV_RECEIVED_MSG | EV_DATA_READABLE);
	return 0;
}

sVFSNode *vfs_server_getWork(sVFSNode *node,bool *cont,bool *retry) {
	sServer *srv = (sServer*)node->data;
	sVFSNode *n,*last;
	/* this is a bit more complicated because we want to do it in a fair way. that means every
	 * process that requests something should be served at some time. therefore we store the last
	 * served client and continue from the next one. */

	/* search for a slot that needs work */
	last = srv->lastClient;
	if(last == NULL)
		n = vfs_node_getFirstChild(node);
	else if(last->next == NULL) {
		/* if we have checked all clients in this driver, give the other drivers
		 * a chance (if there are any others) */
		srv->lastClient = NULL;
		*retry = true;
		return NULL;
	}
	else
		n = last->next;

searchBegin:
	while(n != NULL && n != last) {
		/* data available? */
		if(vfs_chan_hasWork(n)) {
			*cont = false;
			srv->lastClient = n;
			return n;
		}
		n = n->next;
	}
	/* if we have looked through all nodes and the last one has a message again, we have to
	 * store it because we'll not check it again */
	if(last && n == last && vfs_chan_hasWork(n))
		return n;
	if(srv->lastClient) {
		n = vfs_node_getFirstChild(node);
		srv->lastClient = NULL;
		goto searchBegin;
	}
	return NULL;
}

static void vfs_server_wakeupClients(const sVFSNode *node,uint events) {
	node = vfs_node_getFirstChild(node);
	while(node != NULL) {
		ev_wakeupm(events,(tEvObj)node);
		node = node->next;
	}
}

#if DEBUGGING

void vfs_server_dbg_print(const sVFSNode *n) {
	sVFSNode *chan = vfs_node_getFirstChild(n);
	vid_printf("\t%s:\n",n->name);
	while(chan != NULL) {
		vfs_chan_dbg_print(chan);
		chan = chan->next;
	}
	vid_printf("\n");
}

#endif
