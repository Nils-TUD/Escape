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

#include <esc/common.h>
#include <esc/messages.h>
#include <esc/thread.h>
#include <esc/sllist.h>
#include <esc/io.h>
#include <string.h>
#include <stdlib.h>
#include "threadpool.h"

#define RT_STATE_IDLE		0
#define RT_STATE_HASWORK	1
#define RT_STATE_BUSY		2

#define STATE_LOCK			0xF7180003

static int tpool_idle(sReqThread *t);

static volatile bool run = true;
static tTid acceptTid;
static sReqThread threads[REQ_THREAD_COUNT];

void tpool_init(void) {
	u32 i;
	acceptTid = gettid();
	for(i = 0; i < REQ_THREAD_COUNT; i++) {
		s32 tid = startThread((fThreadEntry)tpool_idle,threads + i);
		if(tid < 0)
			error("[FS] Unable to start request-thread %d\n",i);
		threads[i].id = i + 1;
		threads[i].tid = tid;
		threads[i].state = RT_STATE_IDLE;
	}
}

void tpool_shutdown(void) {
	u32 i;
	run = false;
	for(i = 0; i < REQ_THREAD_COUNT; i++)
		notify(threads[i].tid,EV_USER1);
	join(0);
}

u32 tpool_tidToId(tTid tid) {
	u32 i;
	for(i = 0; i < REQ_THREAD_COUNT; i++) {
		if(threads[i].tid == tid)
			return threads[i].id;
	}
	/* then its the initial thread */
	return 0;
}

bool tpool_addRequest(fReqHandler handler,tFD fd,const sMsg *msg,u32 msgSize,void *data) {
	u32 i;
	while(true) {
		lock(STATE_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP);
		for(i = 0; i < REQ_THREAD_COUNT; i++) {
			if(threads[i].state == RT_STATE_IDLE) {
				sFSRequest *req = (sFSRequest*)malloc(sizeof(sFSRequest));
				if(!req) {
					unlock(STATE_LOCK);
					return false;
				}
				req->handler = handler;
				req->fd = fd;
				req->data = data;
				memcpy(&req->msg,msg,msgSize);
				threads[i].req = req;
				threads[i].state = RT_STATE_HASWORK;
				notify(threads[i].tid,EV_USER1);
				unlock(STATE_LOCK);
				return true;
			}
		}
		/* wait until there is a free slot */
		waitUnlock(EV_USER2,0,STATE_LOCK);
	}
	/* unreachable */
	return false;
}

static int tpool_idle(sReqThread *t) {
	while(run) {
		/* wait until we have work */
		while(run && t->state == RT_STATE_IDLE)
			wait(EV_USER1,0);
		if(!run)
			break;

		/* handle request */
		t->state = RT_STATE_BUSY;
		t->req->handler(t->req->fd,&t->req->msg,t->req->data);

		/* clean up */
		close(t->req->fd);
		free(t->req->data);
		free(t->req);
		t->req = NULL;

		/* we're ready for new requests */
		lock(STATE_LOCK,LOCK_EXCLUSIVE | LOCK_KEEP);
		t->state = RT_STATE_IDLE;
		notify(acceptTid,EV_USER2);
		unlock(STATE_LOCK);
	}
	return 0;
}
