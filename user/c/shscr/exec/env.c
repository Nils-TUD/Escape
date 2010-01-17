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

static sVar *env_createVar(const char *name,sValue *val);
static sVar *env_getByName(sEnv *env,const char *name);

sEnv *env_create(void) {
	sEnv *env = (sEnv*)emalloc(sizeof(sEnv));
	env->vars = sll_create();
	/* TODO error-handling */
	env->parent = NULL;
	return env;
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

sValue *env_get(sEnv *env,const char *name) {
	sVar *v = env_getByName(env,name);
	if(v == NULL && env->parent)
		return env_get(env->parent,name);
	if(v)
		return v->val;
	return NULL;
}

sValue *env_set(sEnv *env,const char *name,sValue *val) {
	sVar *v = env_getByName(env,name);
	if(v) {
		efree(v->val);
		v->val = val;
	}
	else {
		v = env_createVar(name,val);
		sll_append(env->vars,v);
		/* TODO error-handling */
	}
	return v->val;
}

void env_destroy(sEnv *env) {
	sll_destroy(env->vars,true);
	efree(env);
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
