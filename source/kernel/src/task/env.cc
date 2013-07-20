/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <sys/common.h>
#include <sys/task/proc.h>
#include <sys/task/env.h>
#include <sys/task/thread.h>
#include <sys/mem/cache.h>
#include <sys/mem/vmm.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <esc/sllist.h>
#include <string.h>

typedef struct {
	uint8_t dup;		/* whether this is a duplicate of a parent-var */
	char *name;
	char *value;
} sEnvVar;

static bool env_exists(const Proc *p,const char *name);
static sEnvVar *env_getiOf(const Proc *p,size_t *index);
static sEnvVar *env_getOf(const Proc *p,const char *name);

bool env_geti(pid_t pid,size_t index,USER char *dst,size_t size) {
	sEnvVar *var;
	while(1) {
		Proc *p = Proc::request(pid,PLOCK_ENV);
		if(!p)
			return false;
		var = env_getiOf(p,&index);
		if(var != NULL) {
			bool res = true;
			if(dst) {
				p->addLock(PLOCK_ENV);
				strnzcpy(dst,var->name,size);
				p->remLock(PLOCK_ENV);
			}
			Proc::release(p,PLOCK_ENV);
			return res;
		}
		if(p->getPid() == 0) {
			Proc::release(p,PLOCK_ENV);
			break;
		}
		pid = p->getParentPid();
		Proc::release(p,PLOCK_ENV);
	}
	return false;
}

bool env_get(pid_t pid,USER const char *name,USER char *dst,size_t size) {
	sEnvVar *var;
	while(1) {
		Proc *p = Proc::request(pid,PLOCK_ENV);
		if(!p)
			return false;
		p->addLock(PLOCK_ENV);
		var = env_getOf(p,name);
		if(var != NULL) {
			if(dst)
				strnzcpy(dst,var->value,size);
			p->remLock(PLOCK_ENV);
			Proc::release(p,PLOCK_ENV);
			return true;
		}
		p->remLock(PLOCK_ENV);
		if(p->getPid() == 0) {
			Proc::release(p,PLOCK_ENV);
			break;
		}
		pid = p->getParentPid();
		Proc::release(p,PLOCK_ENV);
	}
	return false;
}

bool env_set(pid_t pid,USER const char *name,USER const char *value) {
	sEnvVar *var;
	Proc *p;
	char *nameCpy,*valueCpy;
	nameCpy = strdup(name);
	if(!nameCpy)
		return false;
	Thread::addHeapAlloc(nameCpy);
	valueCpy = strdup(value);
	Thread::remHeapAlloc(nameCpy);
	if(!valueCpy)
		goto errorNameCpy;

	/* at first we have to look whether the var already exists for the given process */
	p = Proc::request(pid,PLOCK_ENV);
	if(!p)
		goto errorValCpy;
	var = env_getOf(p,nameCpy);
	if(var != NULL) {
		char *oldVal = var->value;
		/* set value */
		var->value = valueCpy;
		/* we don't need the previous value anymore */
		cache_free(oldVal);
		cache_free(nameCpy);
		Proc::release(p,PLOCK_ENV);
		return true;
	}

	var = (sEnvVar*)cache_alloc(sizeof(sEnvVar));
	if(var == NULL)
		goto errorProc;

	/* we haven't appended the new var yet. so if we find it now, its a duplicate */
	var->dup = env_exists(p,nameCpy);
	/* set name and value */
	var->name = nameCpy;
	var->value = valueCpy;

	/* append to list */
	if(!p->env) {
		p->env = sll_create();
		if(!p->env)
			goto errorVar;
	}
	if(!sll_append(p->env,var))
		goto errorList;
	Proc::release(p,PLOCK_ENV);
	return true;

errorList:
	if(sll_length(p->env) == 0)
		sll_destroy(p->env,false);
errorVar:
	cache_free(var);
errorProc:
	Proc::release(p,PLOCK_ENV);
errorValCpy:
	cache_free(valueCpy);
errorNameCpy:
	cache_free(nameCpy);
	return false;
}

void env_removeFor(pid_t pid) {
	Proc *p = Proc::request(pid,PLOCK_ENV);
	if(p) {
		if(p->env) {
			sSLNode *n;
			for(n = sll_begin(p->env); n != NULL; n = n->next) {
				sEnvVar *var = (sEnvVar*)n->data;
				cache_free(var->name);
				cache_free(var->value);
				cache_free(var);
			}
			sll_destroy(p->env,false);
			p->env = NULL;
		}
		Proc::release(p,PLOCK_ENV);
	}
}

void env_printAllOf(pid_t pid) {
	char name[64];
	char value[64];
	size_t i;
	vid_printf("Environment of %d:\n",pid);
	for(i = 0; ; i++) {
		if(!env_geti(pid,i,name,sizeof(name)))
			break;
		env_get(pid,name,value,sizeof(value));
		vid_printf("\t'%s' = '%s'\n",name,value);
	}
}

static bool env_exists(const Proc *p,const char *name) {
	sEnvVar *var;
	while(1) {
		var = env_getOf(p,name);
		if(var != NULL)
			return true;
		if(p->getPid() == 0)
			break;
		p = Proc::getByPid(p->getParentPid());
	}
	return false;
}

static sEnvVar *env_getiOf(const Proc *p,size_t *index) {
	sSLNode *n;
	sEnvVar *e = NULL;
	if(!p->env)
		return NULL;
	for(n = sll_begin(p->env); n != NULL; n = n->next) {
		e = (sEnvVar*)n->data;
		if(!e->dup && (*index)-- == 0)
			return e;
	}
	return NULL;
}

static sEnvVar *env_getOf(const Proc *p,USER const char *name) {
	sSLNode *n;
	if(!p->env)
		return NULL;
	for(n = sll_begin(p->env); n != NULL; n = n->next) {
		sEnvVar *e = (sEnvVar*)n->data;
		if(strcmp(e->name,name) == 0)
			return e;
	}
	return NULL;
}
