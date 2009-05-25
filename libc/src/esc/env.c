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
#include <esc/proc.h>
#include <esc/io.h>
#include <esc/heap.h>
#include <esc/env.h>
#include <errors.h>
#include <string.h>

/**
 * Opens the env-service
 * @return >= 0 if successfull
 */
static s32 init(void);

/* the fd for the env-service */
static tFD envFd = -1;

/**
 * Sends the given get-message to the env-service and returns the reply-data
 *
 * @param buf the buffer to write to
 * @param bufSize the size of the buffer
 * @param msg the message
 * @return the received data
 */
static bool doGetEnv(char *buf,u32 bufSize,sMsgHeader *msg);

bool getEnvByIndex(char *name,u32 nameSize,u32 index) {
	sMsgHeader *msg;

	if(init() < 0)
		return false;

	msg = asmBinMsg(MSG_ENV_GETI,"22",getpid(),index);
	if(msg == NULL)
		return false;

	return doGetEnv(name,nameSize,msg);
}

bool getEnv(char *value,u32 valSize,const char *name) {
	sMsgHeader *msg;
	u32 nameLen = strlen(name);

	if(init() < 0)
		return false;

	/* build message */
	msg = asmBinDataMsg(MSG_ENV_GET,name,nameLen + 1,"2",getpid());
	if(msg == NULL)
		return false;
	return doGetEnv(value,valSize,msg);
}

s32 setEnv(const char *name,const char* value) {
	u32 nameLen,valLen;
	s32 res;
	char *envVar;
	sMsgHeader *msg;

	if((res = init()) < 0)
		return res;

	nameLen = strlen(name);
	valLen = strlen(value);
	envVar = malloc(nameLen + valLen + 2);
	if(envVar == NULL)
		return ERR_NOT_ENOUGH_MEM;
	strcpy(envVar,name);
	*(envVar + nameLen) = '=';
	strcpy(envVar + nameLen + 1,value);

	/* build message */
	msg = asmBinDataMsg(MSG_ENV_SET,envVar,nameLen + valLen + 2,"2",getpid());
	free(envVar);
	if(msg == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* send message */
	if((res = write(envFd,msg,sizeof(sMsgHeader) + msg->length)) < 0) {
		freeMsg(msg);
		return res;
	}
	freeMsg(msg);
	return 0;
}

static bool doGetEnv(char *buf,u32 bufSize,sMsgHeader *msg) {
	sMsgHeader resp;

	/* send message */
	if(write(envFd,msg,sizeof(sMsgHeader) + msg->length) < 0) {
		freeMsg(msg);
		return false;
	}
	freeMsg(msg);

	/* wait for reply */
	if(read(envFd,&resp,sizeof(sMsgHeader)) < 0)
		return false;

	if(resp.length == 0)
		return false;

	/* read value */
	read(envFd,buf,MIN(bufSize,resp.length));
	if(bufSize < resp.length) {
		/* TODO we need a read to NULL */
		void *tmp = malloc(resp.length - bufSize);
		read(envFd,tmp,resp.length - bufSize);
		free(tmp);
	}
	return true;
}

static s32 init(void) {
	if(envFd >= 0)
		return 0;
	envFd = open("services:/env",IO_READ | IO_WRITE);
	return envFd;
}
