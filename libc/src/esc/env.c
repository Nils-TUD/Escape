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
#include <messages.h>
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

static sMsg msg;
/* the fd for the env-service */
static tFD envFd = -1;

/**
 * Sends the given get-message to the env-service and returns the reply-data
 *
 * @param buf the buffer to write to
 * @param bufSize the size of the buffer
 * @param cmd the command
 * @param size the size of the msg
 * @return the received data
 */
static bool doGetEnv(char *buf,u32 bufSize,u32 cmd,u32 size);

bool getEnvByIndex(char *name,u32 nameSize,u32 index) {
	if(init() < 0)
		return false;

	msg.args.arg1 = getpid();
	msg.args.arg2 = index;
	return doGetEnv(name,nameSize,MSG_ENV_GETI,sizeof(msg.args));
}

bool getEnv(char *value,u32 valSize,const char *name) {
	if(init() < 0)
		return false;

	/* TODO check name-len? */
	msg.str.arg1 = getpid();
	strcpy(msg.str.s1,name);
	return doGetEnv(value,valSize,MSG_ENV_GET,sizeof(msg.str));
}

s32 setEnv(const char *name,const char* value) {
	s32 res;
	if((res = init()) < 0)
		return res;

	/* TODO check lens? */
	msg.str.arg1 = getpid();
	strcpy(msg.str.s1,name);
	strcpy(msg.str.s2,value);

	/* send message */
	return send(envFd,MSG_ENV_SET,&msg,sizeof(msg.str));
}

static bool doGetEnv(char *buf,u32 bufSize,u32 cmd,u32 size) {
	tMsgId mid;

	/* send message */
	if(send(envFd,cmd,&msg,size) < 0)
		return false;

	/* wait for reply */
	if(receive(envFd,&mid,&msg,sizeof(msg)) <= 0)
		return false;

	memcpy(buf,msg.str.s1,MIN(bufSize,msg.str.arg1));
	return msg.str.arg1 > 0;
}

static s32 init(void) {
	if(envFd >= 0)
		return 0;
	envFd = open("/services/env",IO_READ | IO_WRITE);
	return envFd;
}
