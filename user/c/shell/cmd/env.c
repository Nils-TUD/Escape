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
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/env.h>
#include <esc/messages.h>
#include <esc/heap.h>
#include <string.h>
#include "env.h"

s32 shell_cmdEnv(u32 argc,char **argv) {
	if(argc < 2) {
		u32 i,len;
		char *backup;
		char *val,*name;
		for(i = 0; (name = getEnvByIndex(i)) != NULL; i++) {
			/* save name (getEnv will overwrite it) */
			len = strlen(name);
			backup = (char*)malloc(len + 1);
			memcpy(backup,name,len + 1);

			val = getEnv(name);
			if(val != NULL)
				printf("%s=%s\n",backup,val);
		}
	}
	else if(argc == 2) {
		char *val;
		u32 pos = strchri(argv[1],'=');
		if(argv[1][pos] == '\0') {
			val = getEnv(argv[1]);
			if(val != NULL)
				printf("%s=%s\n",argv[1],val);
		}
		else {
			argv[1][pos] = '\0';
			setEnv(argv[1],argv[1] + pos + 1);
			val = getEnv(argv[1]);
			if(val != NULL)
				printf("%s=%s\n",argv[1],val);
		}
	}
	else {
		printf("Usage: %s [<name>|<name>=<value>]\n");
		return 1;
	}

	return 0;
}
