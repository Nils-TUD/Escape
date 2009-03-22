/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <service.h>
#include <heap.h>
#include <messages.h>
#include <string.h>
#include <sllist.h>
#include <proc.h>
#include <signals.h>
#include <debug.h>
#include <io.h>

#define MAP_SIZE 64

typedef struct {
	tPid pid;
	s8 *name;
	s8 *value;
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
static sEnvVar *env_get(tPid pid,s8 *name);

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
static sEnvVar *env_getOf(tPid pid,s8 *name);

/**
 * Fetches the environment-variable with given index for the given process
 *
 * @param pid the process-id
 * @param index the index
 * @return the env-var or NULL
 */
static sEnvVar *env_getiOf(tPid pid,u32 *index);

/**
 * Puts the env-var of the form <name>=<value> in the storage for the given process
 *
 * @param pid the process-id
 * @param env the env-var
 * @return true if successfull
 */
static bool env_put(tPid pid,s8 *env);

/**
 * Sets <name> to <value> for the given process
 *
 * @param pid the process-id
 * @param name the envvar-name
 * @param value the value
 * @return true if successfull
 */
static bool env_set(tPid pid,s8 *name,s8 *value);

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

/* hashmap of linkedlists with env-vars; key=(pid % MAP_SIZE) */
static sSLList *envVars[MAP_SIZE];

s32 main(void) {
	s32 id;

	id = regService("env",SERVICE_TYPE_MULTIPIPE);
	if(id < 0) {
		printLastError();
		return 1;
	}

	if(setSigHandler(SIG_PROC_DIED,procDiedHandler) < 0) {
		printLastError();
		return 1;
	}

	/* set initial vars for proc 0 */
	env_set(0,(s8*)"CWD",(s8*)"file:/");
	env_set(0,(s8*)"PATH",(s8*)"file:/apps/");

	/* wait for messages */
	static sMsgHeader msg;
	while(1) {
		s32 fd = getClient(id);
		if(fd < 0)
			sleep(EV_CLIENT);
		else {
			/* read all available messages */
			while(read(fd,&msg,sizeof(sMsgHeader)) > 0) {
				/* see what we have to do */
				switch(msg.id) {
					case MSG_ENV_GET: {
						u8 *data = (u8*)malloc(msg.length + 1);
						if(data != NULL) {
							if(read(fd,data,msg.length) > 0) {
								tPid pid;
								u32 nameLen;
								s8 *name;
								nameLen = disasmBinDataMsg(msg.length,data,&name,"2",&pid);
								if(nameLen) {
									u32 len;
									sMsgHeader *resp;
									sEnvVar *var;
									*(name + nameLen) = '\0';
									var = env_get(pid,name);

									/* send reply */
									len = var != NULL ? strlen(var->value) + 1 : 0;
									resp = asmDataMsg(MSG_ENV_GET_RESP,len,var != NULL ? var->value : NULL);
									if(resp != NULL) {
										write(fd,resp,sizeof(sMsgHeader) + resp->length);
										freeMsg(resp);
									}
								}
							}
							free(data);
						}
					}
					break;

					case MSG_ENV_GETI: {
						u8 *data = (u8*)malloc(msg.length + 1);
						if(data != NULL) {
							if(read(fd,data,msg.length) > 0) {
								tPid pid;
								u16 index;
								if(disasmBinMsg(data,"22",&pid,&index)) {
									u32 len;
									sMsgHeader *resp;
									sEnvVar *var;
									var = env_geti(pid,index);

									/* send reply */
									len = var != NULL ? strlen(var->name) + 1 : 0;
									resp = asmDataMsg(MSG_ENV_GET_RESP,len,var != NULL ? var->name : NULL);
									if(resp != NULL) {
										write(fd,resp,sizeof(sMsgHeader) + resp->length);
										freeMsg(resp);
									}
								}
							}
							free(data);
						}
					}
					break;

					case MSG_ENV_SET: {
						u8 *data = (u8*)malloc(msg.length + 1);
						if(data != NULL) {
							if(read(fd,data,msg.length) > 0) {
								tPid pid;
								u32 envVarLen;
								s8 *envVar;
								envVarLen = disasmBinDataMsg(msg.length,data,&envVar,"2",&pid);

								/* terminate */
								*(envVar + envVarLen) = '\0';
								env_put(pid,envVar);
							}
						}
						free(data);
					}
					break;
				}
			}
			close(fd);
		}
	}

	/* clean up */
	unregService(id);
	return 0;
}

static void procDiedHandler(tSig sig,u32 data) {
	/* TODO we need a locking-mechanism here */
	/*env_remProc((tPid)data);*/
}

static sEnvVar *env_geti(tPid pid,u32 index) {
	sEnvVar *var;
	while(1) {
		var = env_getiOf(pid,&index);
		if(pid == 0 || var != NULL)
			break;
		pid = getppidof(pid);
	}
	return var;
}

static sEnvVar *env_get(tPid pid,s8 *name) {
	sEnvVar *var;
	while(1) {
		var = env_getOf(pid,name);
		if(pid == 0 || var != NULL)
			break;
		pid = getppidof(pid);
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

static sEnvVar *env_getOf(tPid pid,s8 *name) {
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

static bool env_put(tPid pid,s8 *env) {
	s8 *val = strchr(env,'=');
	if(val == NULL)
		return false;
	*val = '\0';
	return env_set(pid,env,val + 1);
}

static bool env_set(tPid pid,s8 *name,s8 *value) {
	sSLList *list;
	sEnvVar *var;
	u32 len;

	/* at first we have to look wether the var already exists for the given process */
	var = env_getOf(pid,name);
	if(var != NULL) {
		/* we don't need the previous value anymore */
		free(var->value);
		/* set value */
		var->value = (s8*)malloc(len + 1);
		if(var->value == NULL)
			return false;
		memcpy(var->value,value,len + 1);
		var->value = value;
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
	var->name = (s8*)malloc(len + 1);
	if(var->name == NULL)
		return false;
	memcpy(var->name,name,len + 1);
	/* copy value */
	len = strlen(value);
	var->value = (s8*)malloc(len + 1);
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
