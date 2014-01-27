/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#pragma once

#include <esc/common.h>
#include <esc/messages.h>
#include <fs/fsdev.h>

#define REQ_THREAD_COUNT	0

#if REQ_THREAD_COUNT > 0
#define tpool_lock(x,y)		lock(x,y)
#define tpool_unlock(x)		unlock(x)
#else
#define tpool_lock(x,y)		0
#define tpool_unlock(x)		0
#endif

typedef void (*fReqHandler)(sFileSystem *fs,int fd,sMsg *msg,void *data);

typedef struct {
	fReqHandler handler;
	sFileSystem *fs;
	int fd;
	sMsg msg;
	void *data;
} sFSRequest;

typedef struct {
	size_t id;
	tid_t tid;
	volatile uchar state;
	sFSRequest *req;
} sReqThread;

void tpool_init(void);
void tpool_shutdown(void);
size_t tpool_tidToId(tid_t tid);
bool tpool_addRequest(fReqHandler handler,sFileSystem *fs,int fd,const sMsg *msg,
	size_t msgSize,void *data);
