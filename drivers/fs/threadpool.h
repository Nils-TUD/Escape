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

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <esc/common.h>
#include <esc/messages.h>

#define REQ_THREAD_COUNT	8

typedef void (*fReqHandler)(tFD fd,sMsg *msg,void *data);

typedef struct {
	fReqHandler handler;
	tFD fd;
	sMsg msg;
	void *data;
} sFSRequest;

typedef struct {
	u32 id;
	tTid tid;
	volatile u8 state;
	sFSRequest *req;
} sReqThread;

void tpool_init(void);
void tpool_shutdown(void);
bool tpool_hasFreeSlot(void);
u32 tpool_tidToId(tTid tid);
bool tpool_addRequest(fReqHandler handler,tFD fd,const sMsg *msg,u32 msgSize,void *data);

#endif /* THREADPOOL_H_ */
