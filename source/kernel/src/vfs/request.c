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
#include <sys/task/proc.h>
#include <sys/task/event.h>
#include <sys/task/signals.h>
#include <sys/mem/kheap.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/real.h>
#include <sys/vfs/request.h>
#include <sys/util.h>
#include <sys/video.h>
#include <esc/fsinterface.h>
#include <esc/messages.h>
#include <string.h>
#include <errors.h>

/* A small note to this: we can use one channel (identified by the node) in parallel because
 * we expect all drivers to handle the requests in FIFO order. That means, if we order the
 * requests (append new ones at the end), we can simply choose the first with the specified node
 * when looking for a channel that should receive the response.
 * As soon as a request should no longer receive messages, vfs/driver and vfs/real remove the
 * request from the list via vfs_req_remove(). This HAS TO be done by the one that sends the
 * response! Later, when the client got the response, it free's the request.
 */

#define HANDLER_COUNT		32

/* the vfs-driver-file */
static sSLList *requests;
static fReqHandler handler[HANDLER_COUNT] = {NULL};

void vfs_req_init(void) {
	requests = sll_create();
	if(!requests)
		util_panic("Unable to create linked list for requests");
}

bool vfs_req_setHandler(tMsgId id,fReqHandler f) {
	if(id >= HANDLER_COUNT || handler[id] != NULL)
		return false;
	handler[id] = f;
	return true;
}

void vfs_req_sendMsg(tMsgId id,sVFSNode *node,const void *data,size_t size) {
	assert(node != NULL);
	if(id < HANDLER_COUNT && handler[id])
		handler[id](node,data,size);
}

sRequest *vfs_req_get(sVFSNode *node,void *buffer,size_t size) {
	sThread *t = thread_getRunning();
	sRequest *req = NULL;
	assert(node != NULL);

	req = (sRequest*)kheap_alloc(sizeof(sRequest));
	if(!req)
		return NULL;
	req->tid = t->tid;
	req->node = node;
	req->state = REQ_STATE_WAITING;
	req->val1 = 0;
	req->val2 = 0;
	req->data = buffer;
	req->dsize = size;
	req->count = 0;
	if(!sll_append(requests,req)) {
		kheap_free(req);
		return NULL;
	}
	return req;
}

void vfs_req_waitForReply(sRequest *req,bool allowSigs) {
	/* wait */
	ev_wait(req->tid,EVI_REQ_REPLY,(tEvObj)req->node);
	if(allowSigs)
		thread_switch();
	else
		thread_switchNoSigs();
	/* if we waked up and the request is not finished, the driver probably died or we received
	 * a signal (if allowSigs is true) */
	if(req->state != REQ_STATE_FINISHED) {
		/* indicate an error */
		req->count = (allowSigs && sig_hasSignalFor(req->tid)) ? ERR_INTERRUPTED : ERR_DRIVER_DIED;
	}
}

sRequest *vfs_req_getByNode(const sVFSNode *node) {
	sSLNode *n;
	for(n = sll_begin(requests); n != NULL; n = n->next) {
		sRequest *req = (sRequest*)n->data;
		if(req->node == node) {
			/* the thread may have been terminated... */
			if(thread_getById(req->tid) == NULL) {
				vfs_req_free(req);
				return NULL;
			}
			return req;
		}
	}
	return NULL;
}

void vfs_req_remove(sRequest *r) {
	sll_removeFirst(requests,r);
}

void vfs_req_free(sRequest *r) {
	if(r) {
		sll_removeFirst(requests,r);
		kheap_free(r);
	}
}

#if DEBUGGING

void vfs_req_dbg_printAll(void) {
	sSLNode *n;
	vid_printf("Active requests:\n");
	for(n = sll_begin(requests); n != NULL; n = n->next) {
		sRequest *req = (sRequest*)n->data;
		vfs_req_dbg_print(req);
	}
}

void vfs_req_dbg_print(sRequest *r) {
	vid_printf("\tRequest with %#08x (%s):\n",r->node,vfs_node_getPath(vfs_node_getNo(r->node)));
	vid_printf("\t\ttid: %d\n",r->tid);
	vid_printf("\t\tstate: %d\n",r->state);
	vid_printf("\t\tval1: %d\n",r->val1);
	vid_printf("\t\tval2: %d\n",r->val2);
	vid_printf("\t\tdata: %d\n",r->data);
	vid_printf("\t\tdsize: %d\n",r->dsize);
	vid_printf("\t\tcount: %d\n",r->count);
}

#endif
