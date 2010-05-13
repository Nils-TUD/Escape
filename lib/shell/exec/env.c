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
#include <sllist.h>
#include <string.h>
#include "env.h"
#include "../mem.h"

static void env_doGetMatching(sEnv *env,sSLList *list,const char *name,u32 length,bool searchCmd);
static sVar *env_createVar(const char *name,sValue *val);
static sVar *env_getByName(sEnv *env,const char *name);

sEnv *env_create(sEnv *parent) {
	sEnv *env = (sEnv*)emalloc(sizeof(sEnv));
	env->vars = esll_create();
	env->parent = parent;
	return env;
}

void env_addArgs(sEnv *e,s32 count,const char **args) {
	s32 i;
	char name[16];
	for(i = 0; i < count; i++) {
		sprintf(name,"$%d",i);
		env_set(e,name,val_createStr(args[i]));
	}
	env_set(e,"$#",val_createInt(count));
}

void env_print(sEnv *env) {
	sSLNode *n;
	sVar *v;
	printf("Environment [%d elements]:\n",sll_length(env->vars));
	for(n = sll_begin(env->vars); n != NULL; n = n->next) {
		v = (sVar*)n->data;
		printf("  %s = %s\n",v->name,v->val ? val_getStr(v->val) : "NULL");
	}
}

sSLList *env_getMatching(sEnv *env,const char *name,u32 length,bool searchCmd) {
	sSLList *matching = esll_create();
	env_doGetMatching(env,matching,name,length,searchCmd);
	return matching;
}

sValue *env_get(sEnv *env,const char *name) {
	sVar *v = env_getByName(env,name);
	if(v == NULL && env->parent)
		return env_get(env->parent,name);
	if(v)
		return v->val;
	return val_createInt(0);
}

sValue *env_set(sEnv *env,const char *name,sValue *val) {
	sVar *v = env_getByName(env,name);
	if(v) {
		efree(v->val);
		v->val = val;
	}
	else {
		v = env_createVar(name,val);
		esll_append(env->vars,v);
	}
	return v->val;
}

void env_destroy(sEnv *env) {
	sSLNode *n;
	sVar *v;
	for(n = sll_begin(env->vars); n != NULL; n = n->next) {
		v = (sVar*)n->data;
		efree(v->name);
		val_destroy(v->val);
		efree(v);
	}
	sll_destroy(env->vars,false);
	efree(env);
}

static void env_doGetMatching(sEnv *env,sSLList *list,const char *name,u32 length,bool searchCmd) {
	sSLNode *n;
	if(env->parent)
		env_doGetMatching(env->parent,list,name,length,searchCmd);
	for(n = sll_begin(env->vars); n != NULL; n = n->next) {
		sVar *v = (sVar*)n->data;
		u32 varlen = strlen(v->name);
		u32 matchLen = searchCmd ? varlen : length;
		if(length <= varlen && strncmp(name,v->name,matchLen) == 0)
			esll_append(list,v->name);
	}
}

static sVar *env_createVar(const char *name,sValue *val) {
	sVar *v = (sVar*)emalloc(sizeof(sVar));
	v->name = estrdup(name);
	v->val = val;
	return v;
}

static sVar *env_getByName(sEnv *env,const char *name) {
	sSLNode *n;
	sVar *v;
	for(n = sll_begin(env->vars); n != NULL; n = n->next) {
		v = (sVar*)n->data;
		if(strcmp(v->name,name) == 0)
			return v;
	}
	return NULL;
}
