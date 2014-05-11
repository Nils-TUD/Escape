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
#include <esc/cmdargs.h>
#include <esc/dir.h>
#include <esc/io.h>
#include <stdio.h>
#include <stdlib.h>

static void removeRec(const char *path,bool rec) {
	if(isdir(path)) {
		if(!rec) {
			printe("Omitting directory '%s'",path);
			return;
		}

		char tmp[MAX_PATH_LEN];
		DIR *dir = opendir(path);
		sDirEntry e;
		while(readdir(dir,&e)) {
			if(strcmp(e.name,".") == 0 || strcmp(e.name,"..") == 0)
				continue;

			snprintf(tmp,sizeof(tmp),"%s/%s",path,e.name);
			removeRec(tmp,rec);
		}
		closedir(dir);

		if(rmdir(path) < 0)
			printe("Unable to remove directory '%s'",path);
	}
	else if(unlink(path) < 0)
		printe("Unable to unlink '%s'",path);
}

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-r] <path> ...\n",name);
	fprintf(stderr,"    -r:    remove directories recursively\n");
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	int rec = false;
	const char **args;

	int res = ca_parse(argc,argv,0,"r",&rec,&rec);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp() || ca_getFreeCount() == 0)
		usage(argv[0]);

	args = ca_getFree();
	while(*args) {
		removeRec(*args,rec);
		args++;
	}
	return EXIT_SUCCESS;
}
