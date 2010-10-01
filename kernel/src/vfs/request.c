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
#include <sys/task/sched.h>
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

#define FS_RESERVED			32
#define REQUEST_COUNT		1024
#define HANDLER_COUNT		32

/* the vfs-driver-file */
static sRequest requests[REQUEST_COUNT];
static fReqHandler handler[HANDLER_COUNT] = {NULL};

void vfsreq_init(void) {
	u32 i;
	sRequest *req;

	req = requests;
	for(i = 0; i < REQUEST_COUNT; i++) {
		/* a few slots are reserved for fs; because we need driver-requests to handle fs-requests */
		if(i < FS_RESERVED)
			req->tid = KERNEL_TID;
		else
			req->tid = INVALID_TID;
		req->node = NULL;
		req++;
	}
}

bool vfsreq_setHandler(tMsgId id,fReqHandler f) {
	if(id >= HANDLER_COUNT || handler[id] != NULL)
		return false;
	handler[id] = f;
	return true;
}

void vfsreq_sendMsg(tMsgId id,sVFSNode *node,const u8 *data,u32 size) {
	assert(node != NULL);
	if(id < HANDLER_COUNT && handler[id])
		handler[id](node,data,size);
}

sRequest *vfsreq_getRequest(sVFSNode *node,void *buffer,u32 size) {
	u32 i;
	sThread *t = thread_getRunning();
	sRequest *req = NULL;
	assert(node != NULL);

retry:
	for(i = 0; i < REQUEST_COUNT; i++) {
		/* another request with that node? wait! */
		if(requests[i].node == node) {
			req = NULL;
			break;
		}
		if(!req && requests[i].node == NULL &&
				(t->proc->pid == FS_PID || requests[i].tid != KERNEL_TID))
			req = requests + i;
	}
	/* if there is no free slot or another one is using that node, wait */
	if(!req) {
		thread_wait(t->tid,NULL,EV_REQ_FREE);
		thread_switch();
		goto retry;
	}

	req->tid = t->tid;
	req->node = node;
	req->state = REQ_STATE_WAITING;
	req->val1 = 0;
	req->val2 = 0;
	req->data = buffer;
	req->dsize = size;
	req->count = 0;
	return req;
}

void vfsreq_waitForReply(sRequest *req,bool allowSigs) {
	/* wait */
	thread_wait(req->tid,req->node,EV_REQ_REPLY);
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

sRequest *vfsreq_getRequestByNode(sVFSNode *node) {
	u32 i;
	sRequest *req = requests;
	assert(node != NULL);
	for(i = 0; i < REQUEST_COUNT; i++) {
		if(req->node == node) {
			/* the thread may have been terminated... */
			if(thread_getById(req->tid) == NULL) {
				vfsreq_remRequest(req);
				return NULL;
			}
			return req;
		}
		req++;
	}
	return NULL;
}

void vfsreq_remRequest(sRequest *r) {
	r->node = NULL;
	thread_wakeupAll(NULL,EV_REQ_FREE);
}

#if DEBUGGING

void vfsreq_dbg_printAll(void) {
	u32 i;
	vid_printf("Active requests:\n");
	for(i = 0; i < REQUEST_COUNT; i++) {
		if(requests[i].node != NULL)
			vfsreq_dbg_print(requests + i);
	}
}

void vfsreq_dbg_print(sRequest *r) {
	vid_printf("\tRequest with %#08x (%s):\n",r->node,vfsn_getPath(vfsn_getNodeNo(r->node)));
	vid_printf("\t\ttid: %d\n",r->tid);
	vid_printf("\t\tstate: %d\n",r->state);
	vid_printf("\t\tval1: %d\n",r->val1);
	vid_printf("\t\tval2: %d\n",r->val2);
	vid_printf("\t\tdata: %d\n",r->data);
	vid_printf("\t\tdsize: %d\n",r->dsize);
	vid_printf("\t\tcount: %d\n",r->count);
}

#endif
