/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/messages.h"
#include "../h/proc.h"
#include "../h/io.h"
#include "../h/heap.h"
#include "../h/env.h"
#include <string.h>

/**
 * Opens the env-service
 * @return true if successfull
 */
static bool init(void);

/* the fd for the env-service */
static s32 envFd = -1;
static s8 *tmpValue = NULL;

/**
 * Sends the given get-message to the env-service and returns the reply-data
 *
 * @param msg the message
 * @return the received data
 */
static s8 *doGetEnv(sMsgHeader *msg);

s8 *getEnvByIndex(u32 index) {
	sMsgHeader *msg;

	if(!init())
		return NULL;

	msg = asmBinMsg(MSG_ENV_GETI,"22",getpid(),index);
	if(msg == NULL)
		return NULL;

	return doGetEnv(msg);
}

s8 *getEnv(const s8 *name) {
	sMsgHeader *msg;
	u32 nameLen = strlen(name);

	if(!init())
		return NULL;

	/* build message */
	msg = asmBinDataMsg(MSG_ENV_GET,name,nameLen + 1,"2",getpid());
	if(msg == NULL)
		return NULL;
	return doGetEnv(msg);
}

void setEnv(const s8 *name,const s8* value) {
	u32 nameLen,valLen;
	s8 *envVar;
	sMsgHeader *msg;

	if(!init())
		return;

	nameLen = strlen(name);
	valLen = strlen(value);
	envVar = malloc(nameLen + valLen + 2);
	if(envVar == NULL)
		return;
	strcpy(envVar,name);
	*(envVar + nameLen) = '=';
	strcpy(envVar + nameLen + 1,value);

	/* build message */
	msg = asmBinDataMsg(MSG_ENV_SET,envVar,nameLen + valLen + 2,"2",getpid());
	free(envVar);
	if(msg == NULL)
		return;

	/* send message */
	if(write(envFd,msg,sizeof(sMsgHeader) + msg->length) < 0) {
		freeMsg(msg);
		return;
	}
	freeMsg(msg);
}

static s8 *doGetEnv(sMsgHeader *msg) {
	sMsgHeader resp;

	/* send message */
	if(write(envFd,msg,sizeof(sMsgHeader) + msg->length) < 0) {
		freeMsg(msg);
		return NULL;
	}
	freeMsg(msg);

	/* wait for reply */
	if(read(envFd,&resp,sizeof(sMsgHeader)) < 0)
		return NULL;

	/* free previously used value */
	if(tmpValue != NULL)
		free(tmpValue);

	/* read value */
	tmpValue = (s8*)malloc(resp.length);
	if(tmpValue == NULL)
		return NULL;
	read(envFd,tmpValue,resp.length);
	return tmpValue;
}

static bool init(void) {
	if(envFd >= 0)
		return true;
	envFd = open("services:/env",IO_READ | IO_WRITE);
	return envFd >= 0;
}
