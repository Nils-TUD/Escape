/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static char *genvar(const char *name,const char *value,size_t namelen) {
	char *nvar = malloc(namelen + strlen(value) + 2);
	if(!nvar)
		return NULL;
	strncpy(nvar,name,namelen);
	nvar[namelen] = '=';
	strcpy(nvar + namelen + 1,value);
	return nvar;
}

int setenv(const char *name,const char *value) {
	/* try to find the variable */
	char **var = environ;
	size_t len = strlen(name);
	size_t count = 0;
	while(*var) {
		char *val = strchr(*var,'=');
		if(val) {
			/* does the name match? */
			if(len == (size_t)(val - *var) && strncmp(*var,name,len) == 0) {
				/* replace var */
				char *nvar = genvar(name,value,len);
				if(!nvar)
					return -ENOMEM;
				free(*var);
				*var = nvar;
				return 0;
			}
		}
		var++;
		count++;
	}

	/* create new array */
	char **nenv = malloc((count + 2) * sizeof(char*));
	if(!nenv)
		return -ENOMEM;
	/* create new var */
	char *nvar = genvar(name,value,len);
	if(!nvar) {
		free(nenv);
		return -ENOMEM;
	}

	/* copy */
	size_t i;
	for(i = 0; i < count; ++i)
		nenv[i] = environ[i];
	nenv[i] = nvar;
	nenv[i + 1] = NULL;
	/* exchange */
	free(environ);
	environ = nenv;
	return 0;
}
