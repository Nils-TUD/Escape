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

s8 *getenv(s8 *name) {
	u32 nameLen = strlen(name);
	u32 len = sizeof(sMsgDataEnvGetReq) + nameLen + 1;
	sMsgDefHeader resp;
	sMsgDefHeader *msg;
	sMsgDataEnvGetReq *data;

	if(!init())
		return NULL;

	/* build message */
	msg = createDefMsg(MSG_ENV_GET,0,NULL);
	if(msg == NULL)
		return NULL;
	msg->length = len;
	data = (sMsgDataEnvGetReq*)(msg + 1);
	data->pid = getpid();
	memcpy(data->name,name,nameLen + 1);

	/* send message */
	if(write(envFd,msg,sizeof(sMsgDefHeader) + msg->length) < 0) {
		freeDefMsg(msg);
		return NULL;
	}
	freeDefMsg(msg);

	/* wait for reply */
	do {
		sleep(EV_RECEIVED_MSG);
	}
	while(read(envFd,&resp,sizeof(sMsgDefHeader)) < 0);

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

void setenv(s8 *name,s8* value) {
	u32 nameLen,valLen;
	sMsgDefHeader *msg;
	sMsgDataEnvSetReq *data;

	if(!init())
		return NULL;

	/* build message */
	msg = createDefMsg(MSG_ENV_SET,0,NULL);
	if(msg == NULL)
		return NULL;

	nameLen = strlen(name);
	valLen = strlen(value);
	msg->length = sizeof(sMsgDataEnvSetReq) + nameLen + valLen + 2;
	data = (sMsgDataEnvSetReq*)(msg + 1);
	data->pid = getpid();
	strcpy(data->envVar,name);
	*(data->envVar + nameLen) = '=';
	strcpy(data->envVar + nameLen + 1,value);

	/* send message */
	if(write(envFd,msg,sizeof(sMsgDefHeader) + msg->length) < 0) {
		freeDefMsg(msg);
		return NULL;
	}
	freeDefMsg(msg);
}

static bool init(void) {
	if(envFd >= 0)
		return true;
	envFd = open("services:/env",IO_READ | IO_WRITE);
	return envFd >= 0;
}
