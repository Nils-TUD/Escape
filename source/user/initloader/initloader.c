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

#include <esc/common.h>
#include <esc/syscalls.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <dirent.h>
#include <esc/conf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ARG_COUNT		32
#define MAX_ARG_LEN			256

static char **parseArgs(const char *line,int *argc) {
	static char argvals[MAX_ARG_COUNT][MAX_ARG_LEN];
	static char *args[MAX_ARG_COUNT];
	size_t i = 0,j = 0;
	args[0] = argvals[0];
	while(*line) {
		if(*line == ' ') {
			if(args[j][0]) {
				if(j + 2 >= MAX_ARG_COUNT)
					break;
				args[j][i] = '\0';
				j++;
				i = 0;
				args[j] = argvals[j];
			}
		}
		else if(i < MAX_ARG_LEN)
			args[j][i++] = *line;
		line++;
	}
	*argc = j + 1;
	args[j][i] = '\0';
	args[j + 1] = NULL;
	return args;
}

extern char __progname[];

int main(void) {
	/* give ourself a name */
	strcpy(__progname,"initloader");

	if(getpid() != 0)
		error("It's not good to start init twice ;)");

	/* perform some additional initialization in the kernel */
	syscall0(SYSCALL_INIT);

	/* open arguments file */
	char line[256];
	char filepath[MAX_PATH_LEN];
	FILE *args = fopen("/sys/boot/arguments","r");
	if(!args)
		error("Unable to open /sys/boot/arguments for reading");

	/* load boot modules */
	for(int i = 0; !feof(args); ++i) {
		fgetl(line,sizeof(line),args);
		/* skip ourself */
		if(*line == '\0' || i == 0)
			continue;

		int argc;
		char **argv = parseArgs(line,&argc);
		/* ignore modules with no wait-argument (like ramdisks) */
		if(argc < 2)
			continue;

		/* all boot-modules are in /sys/boot */
		snprintf(filepath,sizeof(filepath),"/sys/boot/%s",basename(argv[0]));
		argv[0] = filepath;

		int pid;
		if((pid = fork()) == 0) {
			execv(argv[0],(const char**)argv);
			error("exec failed");
		}
		else if(pid < 0)
			error("fork failed");
		else {
			/* wait for the device to be available */
			struct stat info;
			int res;
			while((res = stat(argv[1],&info)) == -ENOENT)
				sleep(20);
			if(res < 0)
				error("stat for %s failed",argv[1]);
		}
	}
	fclose(args);

	/* mount root device */
	if(sysconfstr(CONF_ROOT_DEVICE,line,sizeof(line)) < 0)
		error("Unable to get root device");
	int fd = open(line,O_RDWRMSG);
	if(fd < 0)
		error("Unable to open '%s'",line);
	if(mount(fd,"/") < 0)
		error("Unable to mount '%s' at '/'",line);

	/* exec init */
	const char *argv[] = {"/bin/init",NULL};
	execv(argv[0],argv);
	return 0;
}
