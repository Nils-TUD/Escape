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

bool Env::geti(pid_t pid,size_t index,USER char *dst,size_t size) {
	EnvVar *var;
	while(1) {
		Proc *p = Proc::request(pid,PLOCK_ENV);
		if(!p)
			return false;
		var = getiOf(p,&index);
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

bool Env::get(pid_t pid,USER const char *name,USER char *dst,size_t size) {
	EnvVar *var;
	while(1) {
		Proc *p = Proc::request(pid,PLOCK_ENV);
		if(!p)
			return false;
		p->addLock(PLOCK_ENV);
		var = getOf(p,name);
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

bool Env::set(pid_t pid,USER const char *name,USER const char *value) {
	EnvVar *var;
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
	var = getOf(p,nameCpy);
	if(var != NULL) {
		char *oldVal = var->value;
		/* set value */
		var->value = valueCpy;
		/* we don't need the previous value anymore */
		Cache::free(oldVal);
		Cache::free(nameCpy);
		Proc::release(p,PLOCK_ENV);
		return true;
	}

	var = (EnvVar*)Cache::alloc(sizeof(EnvVar));
	if(var == NULL)
		goto errorProc;

	/* we haven't appended the new var yet. so if we find it now, its a duplicate */
	var->dup = exists(p,nameCpy);
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
	Cache::free(var);
errorProc:
	Proc::release(p,PLOCK_ENV);
errorValCpy:
	Cache::free(valueCpy);
errorNameCpy:
	Cache::free(nameCpy);
	return false;
}

void Env::removeFor(pid_t pid) {
	Proc *p = Proc::request(pid,PLOCK_ENV);
	if(p) {
		if(p->env) {
			sSLNode *n;
			for(n = sll_begin(p->env); n != NULL; n = n->next) {
				EnvVar *var = (EnvVar*)n->data;
				Cache::free(var->name);
				Cache::free(var->value);
				Cache::free(var);
			}
			sll_destroy(p->env,false);
			p->env = NULL;
		}
		Proc::release(p,PLOCK_ENV);
	}
}

void Env::printAllOf(pid_t pid) {
	char name[64];
	char value[64];
	size_t i;
	vid_printf("Environment of %d:\n",pid);
	for(i = 0; ; i++) {
		if(!geti(pid,i,name,sizeof(name)))
			break;
		get(pid,name,value,sizeof(value));
		vid_printf("\t'%s' = '%s'\n",name,value);
	}
}

bool Env::exists(const Proc *p,const char *name) {
	EnvVar *var;
	while(1) {
		var = getOf(p,name);
		if(var != NULL)
			return true;
		if(p->getPid() == 0)
			break;
		p = Proc::getByPid(p->getParentPid());
	}
	return false;
}

Env::EnvVar *Env::getiOf(const Proc *p,size_t *index) {
	sSLNode *n;
	EnvVar *e = NULL;
	if(!p->env)
		return NULL;
	for(n = sll_begin(p->env); n != NULL; n = n->next) {
		e = (EnvVar*)n->data;
		if(!e->dup && (*index)-- == 0)
			return e;
	}
	return NULL;
}

Env::EnvVar *Env::getOf(const Proc *p,USER const char *name) {
	sSLNode *n;
	if(!p->env)
		return NULL;
	for(n = sll_begin(p->env); n != NULL; n = n->next) {
		EnvVar *e = (EnvVar*)n->data;
		if(strcmp(e->name,name) == 0)
			return e;
	}
	return NULL;
}
