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
#include <sys/io.h>
#include <sys/messages.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../cmds.h"

#define MAX_ENV_LEN		255

int shell_cmdEnv(int argc,char **argv) {
	char *valBuf,*nameBuf;
	if(argc > 2 || getopt_ishelp(argc,argv)) {
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
		int i;
		for(i = 0; getenvito(nameBuf,MAX_ENV_LEN + 1,i) >= 0; i++) {
			if(getenvto(valBuf,MAX_ENV_LEN + 1,nameBuf) >= 0)
				printf("%s=%s\n",nameBuf,valBuf);
		}
	}
	else if(argc == 2) {
		/* set? */
		ssize_t pos = strchri(argv[1],'=');
		if(argv[1][pos] == '\0') {
			if(getenvto(valBuf,MAX_ENV_LEN + 1,argv[1]) >= 0)
				printf("%s=%s\n",argv[1],valBuf);
		}
		/* get */
		else {
			argv[1][pos] = '\0';
			if(setenv(argv[1],argv[1] + pos + 1) < 0)
				printe("Unable to set env-var '%s' to '%s'",argv[1],argv[1] + pos + 1);
			else if(getenvto(valBuf,MAX_ENV_LEN + 1,argv[1]) >= 0)
				printf("%s=%s\n",argv[1],valBuf);
		}
	}

	free(valBuf);
	free(nameBuf);
	return EXIT_SUCCESS;
}
