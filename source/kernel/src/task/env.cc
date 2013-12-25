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
#include <sys/mem/virtmem.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <string.h>

klock_t Env::lock;

bool Env::geti(pid_t pid,size_t index,USER char *dst,size_t size) {
	while(1) {
		EnvVar *var = requestiVarOf(pid,&index);
		if(var != NULL) {
			bool res = true;
			if(dst)
				strnzcpy(dst,var->name,size);
			release();
			return res;
		}

		pid = getPPid(pid);
		if(pid == INVALID_PID)
			break;
	}
	return false;
}

bool Env::get(pid_t pid,USER const char *name,USER char *dst,size_t size) {
	while(1) {
		EnvVar *var = requestVarOf(pid,name);
		if(var != NULL) {
			if(dst)
				strnzcpy(dst,var->value,size);
			release();
			return true;
		}

		pid = getPPid(pid);
		if(pid == INVALID_PID)
			break;
	}
	return false;
}

bool Env::set(pid_t pid,USER const char *name,USER const char *value) {
	char *nameCpy,*valueCpy;
	EnvVar *var;
	Proc *p;
	nameCpy = strdup(name);
	if(!nameCpy)
		return false;
	Thread::addHeapAlloc(nameCpy);
	valueCpy = strdup(value);
	Thread::remHeapAlloc(nameCpy);
	if(!valueCpy)
		goto errorNameCpy;

	/* at first we have to look whether the var already exists for the given process */
	var = requestVarOf(pid,nameCpy);
	if(var != NULL) {
		char *oldVal = var->value;
		/* set value */
		var->value = valueCpy;
		/* we don't need the previous value anymore */
		Cache::free(oldVal);
		Cache::free(nameCpy);
		release();
		return true;
	}

	/* we haven't appended the new var yet. so if we find it now, its a duplicate */
	var = new EnvVar(exists(pid,nameCpy),nameCpy,valueCpy);
	if(var == NULL)
		goto errorValCpy;

	/* append to list */
	SpinLock::acquire(&lock);
	p = Proc::getRef(pid);
	if(p) {
		if(!p->env) {
			p->env = new SList<EnvVar>();
			if(!p->env)
				goto errorVar;
		}
		p->env->append(var);
		Proc::relRef(p);
	}
	SpinLock::release(&lock);
	return true;

errorVar:
	Proc::relRef(p);
	SpinLock::release(&lock);
	delete var;
errorValCpy:
	Cache::free(valueCpy);
errorNameCpy:
	Cache::free(nameCpy);
	return false;
}

void Env::removeFor(pid_t pid) {
	SpinLock::acquire(&lock);
	Proc *p = Proc::getRef(pid);
	if(p) {
		if(p->env) {
			for(auto var = p->env->begin(); var != p->env->end(); ) {
				auto old = var++;
				Cache::free(old->name);
				Cache::free(old->value);
				delete &*old;
			}
			delete p->env;
			p->env = NULL;
		}
		Proc::relRef(p);
	}
	SpinLock::release(&lock);
}

void Env::printAllOf(OStream &os,pid_t pid) {
	char name[64];
	char value[64];
	os.writef("Environment of %d:\n",pid);
	for(size_t i = 0; ; i++) {
		if(!geti(pid,i,name,sizeof(name)))
			break;
		get(pid,name,value,sizeof(value));
		os.writef("\t'%s' = '%s'\n",name,value);
	}
}

bool Env::exists(pid_t pid,const char *name) {
	while(1) {
		EnvVar *var = requestVarOf(pid,name);
		if(var != NULL) {
			release();
			return true;
		}

		pid = getPPid(pid);
		if(pid == INVALID_PID)
			break;
	}
	return false;
}

Env::EnvVar *Env::requestiVarOf(pid_t pid,size_t *index) {
	SList<EnvVar> *env = request(pid);
	if(env) {
		for(auto var = env->begin(); var != env->end(); ++var) {
			if(!var->dup && (*index)-- == 0)
				return &*var;
		}
	}
	release();
	return NULL;
}

Env::EnvVar *Env::requestVarOf(pid_t pid,USER const char *name) {
	SList<EnvVar> *env = request(pid);
	if(env) {
		for(auto var = env->begin(); var != env->end(); ++var) {
			if(strcmp(var->name,name) == 0)
				return &*var;
		}
	}
	release();
	return NULL;
}

pid_t Env::getPPid(pid_t pid) {
	if(pid == 0)
		return INVALID_PID;
	const Proc *p = Proc::getRef(pid);
	if(!p)
		return INVALID_PID;
	pid = p->getParentPid();
	Proc::relRef(p);
	return pid;
}

SList<Env::EnvVar> *Env::request(pid_t pid) {
	SpinLock::acquire(&lock);
	Thread::addLock(&lock);
	const Proc *p = Proc::getRef(pid);
	if(!p)
		return NULL;

	SList<EnvVar> *env = p->env;
	Proc::relRef(p);
	return env;
}

void Env::release() {
	Thread::remLock(&lock);
	SpinLock::release(&lock);
}
