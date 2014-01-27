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
#include <esc/thread.h>
#include <esc/dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../modules.h"

#define THREAD_COUNT	16
#define BUF_SIZE		4096

static int threadFunc(void *arg);

int mod_fsreads(int argc,char *argv[]) {
	sDirEntry e;
	DIR *dir;
	size_t count = argc >= 3 ? atoi(argv[2]) : 10;
	size_t i,j;
	for(i = 0; i < THREAD_COUNT; i++) {
		if(startthread(threadFunc,strdup("/zeros")) < 0)
			error("Unable to start thread");
	}

	for(j = 0; j < count; j++) {
		dir = opendir("/bin");
		if(!dir)
			error("Unable to open dir '/bin'");
		for(i = 0; i < THREAD_COUNT; i++) {
			char *path;
			if(!readdir(dir,&e))
				break;
			if(strcmp(e.name,".") == 0 || strcmp(e.name,"..") == 0)
				continue;
			path = (char*)malloc(MAX_PATH_LEN);
			if(!path)
				error("Not enough memory");
			strcpy(path,"/bin/");
			strcat(path,e.name);
			if(startthread(threadFunc,(void*)path) < 0)
				error("Unable to start thread");
		}
		closedir(dir);
	}
	join(0);
	return 0;
}

static int threadFunc(void *arg) {
	char buffer[BUF_SIZE];
	FILE *f = fopen((char*)arg,"r");
	ssize_t count;
	if(!f) {
		free(arg);
		error("Unable to open '%s'",(char*)arg);
	}
	while((count = fread(buffer,1,BUF_SIZE,f)) > 0)
		;
	fclose(f);
	free(arg);
	return 0;
}
