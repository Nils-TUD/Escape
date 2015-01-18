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

#include <esc/proto/vterm.h>
#include <esc/env.h>
#include <sys/cmdargs.h>
#include <sys/common.h>
#include <sys/messages.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#define KEYMAP_DIR		"/etc/keymaps"

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-s <name>]\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,const char **argv) {
	char *kmname = NULL;

	int res = ca_parse(argc,argv,CA_NO_FREE,"s=s",&kmname);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	esc::VTerm vterm(esc::env::get("TERM").c_str());

	/* set keymap? */
	if(kmname != NULL) {
		std::string keymap = std::string(KEYMAP_DIR) + "/" + kmname;
		vterm.setKeymap(keymap);
		printf("Successfully changed keymap to '%s'\n",keymap.c_str());
	}
	/* list all keymaps */
	else {
		struct stat curInfo;
		std::string keymap = vterm.getKeymap();
		if(stat(keymap.c_str(),&curInfo) < 0)
			printe("Unable to stat current keymap (%s)",keymap.c_str());

		struct dirent e;
		DIR *dir = opendir(KEYMAP_DIR);
		if(!dir)
			error("Unable to open '%s'",KEYMAP_DIR);
		while(readdir(dir,&e)) {
			if(strcmp(e.d_name,".") != 0 && strcmp(e.d_name,"..") != 0) {
				char fpath[MAX_PATH_LEN];
				snprintf(fpath,sizeof(fpath),"%s/%s",KEYMAP_DIR,e.d_name);
				struct stat finfo;
				if(stat(fpath,&finfo) < 0)
					printe("Unable to stat '%s'",fpath);

				if(finfo.st_ino == curInfo.st_ino && finfo.st_dev == curInfo.st_dev)
					printf("* %s\n",e.d_name);
				else
					printf("  %s\n",e.d_name);
			}
		}
		closedir(dir);
	}
	return EXIT_SUCCESS;
}
