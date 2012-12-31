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

#pragma once

#include <esc/common.h>
#include <esc/sllist.h>
#include "value.h"

/* an environment */
typedef struct sEnv sEnv;
struct sEnv {
	sSLList *vars;
	sEnv *parent;
};

/* a variable: name => value */
typedef struct {
	char *name;
	sValue *val;
} sVar;

/**
 * Creates a new environment with given parent-env
 *
 * @param parent the parent-env or NULL if its the root-env
 * @return the environment
 */
sEnv *env_create(sEnv *parent);

/**
 * Adds the given arguments as $i and $# into the given environment
 *
 * @param e the env
 * @param count the arg-count
 * @param args the arguments
 */
void env_addArgs(sEnv *e,int count,const char **args);

/**
 * Prints the contents of the given environment
 *
 * @param env the env
 */
void env_print(sEnv *env);

/**
 * Collects all matching variable-names in the given environment
 *
 * @param env the env
 * @param name the name to find (or the beginning of it)
 * @param length the length of the name
 * @param searchCmd whether you want to have a command that can be executed
 * @return a linked list with all names; you have to free it with sll_destroy(list,false);
 */
sSLList *env_getMatching(sEnv *env,const char *name,size_t length,bool searchCmd);

/**
 * Returns the value of the given variable in the given environment. If not present, it looks
 * it up in the parent-env. If not present at all, it returns NULL. Note that the variable is not
 * yours, so you can't free it!
 *
 * @param env the env
 * @param name the name
 * @return the value or NULL
 */
sValue *env_get(sEnv *env,const char *name);

/**
 * Sets the given variable to given value
 *
 * @param env the env
 * @param name the name
 * @param val the value (will not be copied!)
 * @return the set value
 */
sValue *env_set(sEnv *env,const char *name,sValue *val);

/**
 * Destroys the given environment
 *
 * @param env the env
 */
void env_destroy(sEnv *env);
