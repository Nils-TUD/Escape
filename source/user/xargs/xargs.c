/**
 * $Id: wcmain.c 1078 2011-10-11 00:09:24Z nasmussen $
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

#include <esc/common.h>
#include <esc/dir.h>
#include <esc/proc.h>
#include <esc/cmdargs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_ARG_LEN		1024

typedef struct sArg {
	const char *arg;
	struct sArg *next;
} sArg;

static void appendArg(void);
static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-a <file>] [<command>] [<arg>...]\n",name);
	fprintf(stderr,"    By default, <command>=/bin/echo\n");
	exit(EXIT_FAILURE);
}

static sArg *first = NULL;
static sArg *last = NULL;
static size_t argPos = 0;
static char argBuf[MAX_ARG_LEN];

int main(int argc,const char *argv[]) {
	char c;
	const char **args;
	size_t i,argCount;
	size_t freeArgs;
	FILE *file = stdin;
	const char *filename = NULL;

	/* parse args */
	int res = ca_parse(argc,argv,0,"a=s",&filename);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	/* open file? */
	if(filename != NULL) {
		file = fopen(filename,"r");
		if(file == NULL)
			error("Unable to open file '%s'",filename);
	}

	/* read arguments */
	argCount = 0;
	while((c = fgetc(file)) != EOF) {
		if(isspace(c)) {
			appendArg();
			argCount++;
			argPos = 0;
		}
		else {
			if(argPos < MAX_ARG_LEN)
				argBuf[argPos++] = c;
			else
				error("Argument too long");
		}
	}
	if(ferror(file))
		error("Read failed");
	if(filename != NULL)
		fclose(file);
	/* append last one */
	if(argPos > 0) {
		appendArg();
		argCount++;
	}

	/* build args-array and copy arguments into it */
	freeArgs = ca_getFreeCount();
	if(freeArgs == 0)
		freeArgs = 1;
	args = (const char**)malloc(sizeof(char*) * (argCount + freeArgs + 1));
	if(!args)
		error("Not enough mem");
	i = 0;
	if(ca_getFreeCount() == 0)
		args[i++] = "echo";
	else {
		for(i = 0; i < freeArgs; i++)
			args[i] = ca_getFree()[i];
	}
	for(; first != NULL; i++) {
		args[i] = first->arg;
		first = first->next;
	}
	args[i] = NULL;
	/* exchange us with requested command */
	execvp(args[0],args);
	return EXIT_SUCCESS;
}

static void appendArg(void) {
	sArg *a = (sArg*)malloc(sizeof(sArg));
	if(!a)
		error("Not enough mem");
	argBuf[argPos] = '\0';
	a->arg = strdup(argBuf);
	if(!a->arg)
		error("Not enough mem");
	a->next = NULL;
	if(last)
		last->next = a;
	else
		first = a;
	last = a;
}
