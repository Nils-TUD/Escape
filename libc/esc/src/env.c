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
 * @return >= 0 if successfull
 */
static s32 init(void);

/* the fd for the env-service */
static tFD envFd = -1;
static char *tmpValue = NULL;

/**
 * Sends the given get-message to the env-service and returns the reply-data
 *
 * @param msg the message
 * @return the received data
 */
static char *doGetEnv(sMsgHeader *msg);

char *getEnvByIndex(u32 index) {
	sMsgHeader *msg;

	if(init() < 0)
		return NULL;

	msg = asmBinMsg(MSG_ENV_GETI,"22",getpid(),index);
	if(msg == NULL)
		return NULL;

	return doGetEnv(msg);
}

char *getEnv(const char *name) {
	sMsgHeader *msg;
	u32 nameLen = strlen(name);

	if(init() < 0)
		return NULL;

	/* build message */
	msg = asmBinDataMsg(MSG_ENV_GET,name,nameLen + 1,"2",getpid());
	if(msg == NULL)
		return NULL;
	return doGetEnv(msg);
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

static char *doGetEnv(sMsgHeader *msg) {
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
	tmpValue = (char*)malloc(resp.length);
	if(tmpValue == NULL)
		return NULL;
	read(envFd,tmpValue,resp.length);
	return tmpValue;
}

static s32 init(void) {
	if(envFd >= 0)
		return 0;
	envFd = open("services:/env",IO_READ | IO_WRITE);
	return envFd;
}
