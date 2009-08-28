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
#include <esc/service.h>
#include <esc/heap.h>
#include <messages.h>
#include <esc/proc.h>
#include <esc/signals.h>
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <stdlib.h>
#include <string.h>
#include <sllist.h>

#define MAP_SIZE 64

typedef struct {
	tPid pid;
	char *name;
	char *value;
} sEnvVar;

/**
 * Our signal-handler for dead processes
 */
static void procDiedHandler(tSig sig,u32 data);

/**
 * Fetches the environment-variable with given name for the given process. If it does not exist
 * it walks through the parents.
 *
 * @param pid the process-id
 * @param name the envvar-name
 * @return the env-var or NULL
 */
static sEnvVar *env_get(tPid pid,char *name);

/**
 * Fetches the environment-variable with given index for the given process. If it does not exist
 * it walks through the parents.
 *
 * @param pid the process-id
 * @param index the index
 * @return the env-var or NULL
 */
static sEnvVar *env_geti(tPid pid,u32 index);

/**
 * Fetches the environment-variable for given process.
 *
 * @param pid the process-id
 * @param name the envvar-name
 * @return the env-var or NULL
 */
static sEnvVar *env_getOf(tPid pid,char *name);

/**
 * Fetches the environment-variable with given index for the given process
 *
 * @param pid the process-id
 * @param index the index
 * @return the env-var or NULL
 */
static sEnvVar *env_getiOf(tPid pid,u32 *index);

/**
 * Sets <name> to <value> for the given process
 *
 * @param pid the process-id
 * @param name the envvar-name
 * @param value the value
 * @return true if successfull
 */
static bool env_set(tPid pid,char *name,char *value);

/**
 * Removes all envvars for the given process
 *
 * @param pid the process-id
 */
static void env_remProc(tPid pid);

/**
 * Prints all env-vars
 */
static void env_printAll(void);

/* a list of dead processes whose entries should be removed */
static sSLList *deadProcs = NULL;

/* hashmap of linkedlists with env-vars; key=(pid % MAP_SIZE) */
static sSLList *envVars[MAP_SIZE];

static sMsg msg;

int main(void) {
	tServ id,client;
	tMsgId mid;

	id = regService("env",SERV_DEFAULT);
	if(id < 0) {
		printe("Unable to register service 'env'");
		return EXIT_FAILURE;
	}

	if(setSigHandler(SIG_PROC_DIED,procDiedHandler) < 0) {
		printe("Unable to set sig-handler for %d",SIG_PROC_DIED);
		return EXIT_FAILURE;
	}

	/* set initial vars for proc 0 */
	env_set(0,(char*)"CWD",(char*)"/");
	env_set(0,(char*)"PATH",(char*)"/bin/");

	/* wait for messages */
	while(1) {
		tFD fd = getClient(&id,1,&client);
		if(fd < 0)
			wait(EV_CLIENT);
		else {
			/* first, delete dead processes if there are any */
			if(sll_length(deadProcs) > 0) {
				sSLNode *n;
				for(n = sll_begin(deadProcs); n != NULL; n = n->next)
					env_remProc((tPid)(n->data));
				sll_removeAll(deadProcs);
			}

			/* read all available messages */
			while(receive(fd,&mid,&msg,sizeof(msg)) > 0) {
				/* see what we have to do */
				switch(mid) {
					case MSG_ENV_GET: {
						sEnvVar *var;
						u32 pid = msg.str.arg1;
						char *name = msg.str.s1;
						name[sizeof(msg.str.s1) - 1] = '\0';

						var = env_get(pid,name);

						msg.str.arg1 = 0;
						if(var != NULL) {
							msg.str.arg1 = strlen(var->value) + 1;
							memcpy(msg.str.s1,var->value,msg.str.arg1);
						}
						send(fd,MSG_ENV_GET_RESP,&msg,sizeof(msg.str));
					}
					break;

					case MSG_ENV_SET: {
						u32 pid = msg.str.arg1;
						char *name = msg.str.s1;
						char *value = msg.str.s2;
						name[sizeof(msg.str.s1) - 1] = '\0';
						value[sizeof(msg.str.s2) - 1] = '\0';

						env_set(pid,name,value);
					}
					break;

					case MSG_ENV_GETI: {
						sEnvVar *var;
						u32 pid = msg.args.arg1;
						u32 index = msg.args.arg2;

						var = env_geti(pid,index);

						msg.str.arg1 = 0;
						if(var != NULL) {
							msg.str.arg1 = strlen(var->name) + 1;
							memcpy(msg.str.s1,var->name,msg.str.arg1);
						}
						send(fd,MSG_ENV_GET_RESP,&msg,sizeof(msg.str));
					}
					break;
				}
			}
			close(fd);
		}
	}

	/* clean up */
	unregService(id);
	return EXIT_SUCCESS;
}

static void procDiedHandler(tSig sig,u32 data) {
	UNUSED(sig);
	/* remember for deletion */
	if(deadProcs == NULL)
		deadProcs = sll_create();
	if(deadProcs != NULL)
		sll_append(deadProcs,(void*)data);
}

static sEnvVar *env_geti(tPid pid,u32 index) {
	s32 spid = pid;
	sEnvVar *var;
	while(1) {
		var = env_getiOf(spid,&index);
		if(spid <= 0 || var != NULL)
			break;
		spid = getppidof(spid);
	}
	return var;
}

static sEnvVar *env_get(tPid pid,char *name) {
	s32 spid = pid;
	sEnvVar *var;
	while(1) {
		var = env_getOf(spid,name);
		if(spid <= 0 || var != NULL)
			break;
		spid = getppidof(spid);
	}
	return var;
}

static sEnvVar *env_getiOf(tPid pid,u32 *index) {
	sSLNode *n;
	sSLList *list = envVars[pid % MAP_SIZE];
	if(list != NULL) {
		sEnvVar *e = NULL;
		for(n = sll_begin(list); n != NULL; n = n->next) {
			e = (sEnvVar*)n->data;
			if(e->pid == pid) {
				if((*index)-- == 0)
					break;
			}
		}
		if(n == NULL)
			return NULL;
		return e;
	}
	return NULL;
}

static sEnvVar *env_getOf(tPid pid,char *name) {
	sSLNode *n;
	sSLList *list = envVars[pid % MAP_SIZE];
	if(list != NULL) {
		for(n = sll_begin(list); n != NULL; n = n->next) {
			sEnvVar *e = (sEnvVar*)n->data;
			if(e->pid == pid && strcmp(e->name,name) == 0)
				return e;
		}
	}
	return NULL;
}

static bool env_set(tPid pid,char *name,char *value) {
	sSLList *list;
	sEnvVar *var;
	u32 len;

	/* at first we have to look wether the var already exists for the given process */
	var = env_getOf(pid,name);
	if(var != NULL) {
		/* we don't need the previous value anymore */
		free(var->value);
		/* set value */
		len = strlen(value);
		var->value = (char*)malloc(len + 1);
		if(var->value == NULL)
			return false;
		memcpy(var->value,value,len + 1);
		return true;
	}

	var = (sEnvVar*)malloc(sizeof(sEnvVar));
	if(var == NULL)
		return false;

	/* get list */
	list = envVars[pid % MAP_SIZE];
	if(list == NULL) {
		list = sll_create();
		envVars[pid % MAP_SIZE] = list;
	}

	/* copy name */
	len = strlen(name);
	var->pid = pid;
	var->name = (char*)malloc(len + 1);
	if(var->name == NULL)
		return false;
	memcpy(var->name,name,len + 1);
	/* copy value */
	len = strlen(value);
	var->value = (char*)malloc(len + 1);
	if(var->value == NULL)
		return false;
	memcpy(var->value,value,len + 1);

	/* append to list */
	sll_append(list,var);
	return true;
}

static void env_remProc(tPid pid) {
	sSLNode *n,*t,*p;
	sSLList *list = envVars[pid % MAP_SIZE];
	if(list != NULL) {
		p = NULL;
		for(n = sll_begin(list); n != NULL; ) {
			sEnvVar *e = (sEnvVar*)n->data;
			if(e->pid == pid) {
				t = n->next;
				free(e->name);
				free(e->value);
				free(e);
				sll_removeNode(list,n,p);
				n = t;
			}
			else {
				p = n;
				n = n->next;
			}
		}
	}
}

static void env_printAll(void) {
	u32 i;
	sSLNode *n;
	sSLList **list = envVars;
	debugf("Env-Vars:\n");
	for(i = 0; i < MAP_SIZE; i++) {
		if(*list != NULL) {
			for(n = sll_begin(*list); n != NULL; n = n->next) {
				sEnvVar *e = (sEnvVar*)n->data;
				debugf("\t[pid=%d] %s=%s\n",e->pid,e->name,e->value);
			}
		}
		list++;
	}
}
