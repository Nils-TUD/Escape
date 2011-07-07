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

#include <esc/common.h>
#include <esc/thread.h>
#include <esc/dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fsreads.h"

#define THREAD_COUNT	16
#define BUF_SIZE		4096

static int threadFunc(void *arg);

int mod_fsreads(int argc,char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);
	size_t i;
	for(i = 0; i < THREAD_COUNT; i++) {
		if(startThread(threadFunc,(void*)"/zeros") < 0)
			error("Unable to start thread");
	}
	sDirEntry e;
	DIR *dir = opendir("/bin");
	for(i = 0; i < THREAD_COUNT; i++) {
		char *path;
		if(!readdir(dir,&e))
			break;
		if(strcmp(e.name,".") == 0 || strcmp(e.name,"..") == 0)
			continue;
		path = (char*)malloc(MAX_PATH_LEN);
		strcpy(path,"/bin/");
		strcat(path,e.name);
		if(startThread(threadFunc,(void*)path) < 0)
			error("Unable to start thread");
	}
	closedir(dir);
	join(0);
	return 0;
}

static int threadFunc(void *arg) {
	char buffer[BUF_SIZE];
	FILE *f = fopen((char*)arg,"r");
	ssize_t count;
	if(!f)
		error("Unable to open '%s'",(char*)arg);
	while((count = fread(buffer,1,BUF_SIZE,f)) > 0)
		;
	fclose(f);
	return 0;
}
