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
#include <esc/proc.h>
#include <esc/thread.h>
#include <stdio.h>
#include <string.h>
#include "pipe.h"

static void pipeReadChild(void);
static void pipeReadParent(void);
static void pipeChild2Child(void);
static void pipeThrough(void);

int mod_pipe(int argc,char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);
	printf("Reading from child...\n");
	fflush(stdout);
	pipeReadChild();
	printf("Done\n\n");
	printf("Reading from parent...\n");
	fflush(stdout);
	pipeReadParent();
	printf("Done\n\n");
	printf("Letting child 'ls' write to child 'wc'...\n");
	fflush(stdout);
	pipeChild2Child();
	printf("Done\n\n");
	printf("Letting child 'ls' write to child 'wc' and read it from the parent...\n");
	fflush(stdout);
	pipeThrough();
	printf("Done\n\n");
	return 0;
}

static void pipeReadChild(void) {
	tFD fd[2];
	tPid child;
	char buf[10];
	s32 c;
	if(pipe(fd,fd + 1) < 0)
		error("Unable to create pipe");

	child = fork();
	if(child == 0) {
		/* child */
		close(fd[0]);
		for(c = 0; c < 10; c++) {
			snprintf(buf,10,"test%d",c);
			if(write(fd[1],buf,strlen(buf)) < 0)
				error("Write failed");
			sleep(10);
		}
		close(fd[1]);
		exit(0);
	}
	else if(child > 0) {
		/* parent */
		close(fd[1]);
		while((c = RETRY(read(fd[0],buf,9))) > 0) {
			buf[c] = '\0';
			printf("Read '%s'\n",buf);
		}
		close(fd[0]);
		waitChild(NULL);
	}
	else
		error("fork() failed");
}

static void pipeReadParent(void) {
	tFD fd[2];
	tPid child;
	char buf[10];
	s32 c;
	if(pipe(fd,fd + 1) < 0)
		error("Unable to create pipe");

	child = fork();
	if(child == 0) {
		/* child */
		close(fd[1]);
		while((c = RETRY(read(fd[0],buf,9))) > 0) {
			buf[c] = '\0';
			printf("Read '%s'\n",buf);
		}
		close(fd[0]);
		exit(0);
	}
	else if(child > 0) {
		/* parent */
		close(fd[0]);
		for(c = 0; c < 10; c++) {
			snprintf(buf,10,"test%d",c);
			if(write(fd[1],buf,strlen(buf)) < 0)
				error("Write failed");
			sleep(10);
		}
		close(fd[1]);
		waitChild(NULL);
	}
	else
		error("fork() failed");
}

static void pipeChild2Child(void) {
	tFD fd[2];
	tPid child;

	if(pipe(fd,fd + 1) < 0)
		error("Unable to create pipe");

	child = fork();
	if(child == 0) {
		const char *args[] = {"/bin/ls",NULL};
		/* child */
		close(fd[0]);
		redirFd(STDOUT_FILENO,fd[1]);
		exec(args[0],args);
		error("exec failed");
	}
	else if(child > 0) {
		/* parent */
		child = fork();
		if(child == 0) {
			const char *args[] = {"/bin/wc",NULL};
			/* child */
			close(fd[1]);
			redirFd(STDIN_FILENO,fd[0]);
			exec(args[0],args);
			error("exec failed");
		}
		else if(child > 0) {
			/* parent */
			close(fd[0]);
			close(fd[1]);
			waitChild(NULL);
			waitChild(NULL);
		}
		else
			error("inner fork() failed");
	}
	else
		error("outer fork() failed");
}

static void pipeThrough(void) {
	tFD fd[4];
	tPid child;

	if(pipe(fd,fd + 1) < 0)
		error("Unable to create pipe");

	child = fork();
	if(child == 0) {
		const char *args[] = {"/bin/ls",NULL};
		/* child */
		close(fd[0]);
		redirFd(STDOUT_FILENO,fd[1]);
		exec(args[0],args);
		error("exec failed");
	}
	else if(child > 0) {
		/* parent */
		close(fd[1]);
		if(pipe(fd + 2,fd + 3) < 0)
			error("Unable to create pipe");

		child = fork();
		if(child == 0) {
			/* child */
			close(fd[2]);
			const char *args[] = {"/bin/wc",NULL};
			redirFd(STDOUT_FILENO,fd[3]);
			redirFd(STDIN_FILENO,fd[0]);
			exec(args[0],args);
			error("exec failed");
		}
		else if(child > 0) {
			/* parent */
			s32 c;
			char buf[10];
			close(fd[0]);
			close(fd[3]);
			while((c = RETRY(read(fd[2],buf,9))) > 0) {
				buf[c] = '\0';
				printf("Read '%s'\n",buf);
			}
			close(fd[2]);
			waitChild(NULL);
			waitChild(NULL);
		}
		else
			error("inner fork() failed");
	}
	else
		error("outer fork() failed");
}
