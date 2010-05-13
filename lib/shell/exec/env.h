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

#ifndef EXECENV_H_
#define EXECENV_H_

#include <esc/common.h>
#include <sllist.h>
#include "value.h"

typedef struct sEnv sEnv;
struct sEnv {
	sSLList *vars;
	sEnv *parent;
};

typedef struct {
	char *name;
	sValue *val;
} sVar;

sEnv *env_create(sEnv *parent);

void env_print(sEnv *env);

sSLList *env_getMatching(sEnv *env,const char *name,u32 length,bool searchCmd);

sValue *env_get(sEnv *env,const char *name);

sValue *env_set(sEnv *env,const char *name,sValue *val);

void env_destroy(sEnv *env);

#endif /* EXECENV_H_ */
