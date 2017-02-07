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

#include <sys/common.h>
#include <sys/proc.h>
#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc,char *argv[]) {
	const char *filename = NULL;

	/* parse args */
	int opt;
	while((opt = getopt(argc,argv,"a:")) != -1) {
		switch(opt) {
			case 'a': filename = optarg; break;
			default:
				usage(argv[0]);
		}
	}

	/* open file? */
	FILE *file = stdin;
	if(filename != NULL) {
		file = fopen(filename,"r");
		if(file == NULL)
			error("Unable to open file '%s'",filename);
	}

	/* read arguments */
	size_t argCount = 0;
	char c;
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
	size_t freeArgs = optind >= argc ? 1 : argc - optind;
	const char **args = (const char**)malloc(sizeof(char*) * (argCount + freeArgs + 1));
	if(!args)
		error("Not enough mem");
	int j = 0;
	if(optind >= argc)
		args[j++] = "echo";
	else {
		for(int i = optind; i < argc; ++i)
			args[j++] = argv[i];
	}
	for(; first != NULL; j++) {
		args[j] = first->arg;
		first = first->next;
	}
	args[j] = NULL;
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
