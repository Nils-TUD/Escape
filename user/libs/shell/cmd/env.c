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
#include <esc/cmdargs.h>
#include <esc/heap.h>
#include <messages.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "env.h"

#define MAX_ENV_LEN		255

s32 shell_cmdEnv(u32 argc,char **argv) {
	char *valBuf,*nameBuf;
	if(argc > 2 || isHelpCmd(argc,argv)) {
		printf("Usage: %s [<name>|<name>=<value>]\n",argv[0]);
		return EXIT_FAILURE;
	}

	/* allocate mem */
	valBuf = (char*)malloc((MAX_ENV_LEN + 1) * sizeof(char));
	nameBuf = (char*)malloc((MAX_ENV_LEN + 1) * sizeof(char));
	if(valBuf == NULL || nameBuf == NULL) {
		printe("Unable to allocate mem");
		return EXIT_FAILURE;
	}

	/* list all env-vars */
	if(argc < 2) {
		u32 i;
		for(i = 0; getenvito(nameBuf,MAX_ENV_LEN + 1,i); i++) {
			if(getenvto(valBuf,MAX_ENV_LEN + 1,nameBuf))
				printf("%s=%s\n",nameBuf,valBuf);
		}
	}
	else if(argc == 2) {
		/* set? */
		u32 pos = strchri(argv[1],'=');
		if(argv[1][pos] == '\0') {
			if(getenvto(valBuf,MAX_ENV_LEN + 1,argv[1]))
				printf("%s=%s\n",argv[1],valBuf);
		}
		/* get */
		else {
			argv[1][pos] = '\0';
			setenv(argv[1],argv[1] + pos + 1);
			if(getenvto(valBuf,MAX_ENV_LEN + 1,argv[1]))
				printf("%s=%s\n",argv[1],valBuf);
		}
	}

	free(valBuf);
	free(nameBuf);
	return EXIT_SUCCESS;
}
