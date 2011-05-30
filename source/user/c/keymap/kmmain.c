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
#include <esc/proc.h>
#include <esc/messages.h>
#include <esc/cmdargs.h>
#include <esc/dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEYMAP_DIR		"/etc/keymaps"

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [--set <name>]\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,const char **argv) {
	char *kmname = NULL;

	int res = ca_parse(argc,argv,CA_NO_FREE,"set=s",&kmname);
	if(res < 0) {
		fprintf(stderr,"Invalid arguments: %s\n",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	/* set keymap? */
	if(kmname != NULL) {
		char path[MAX_PATH_LEN];
		sMsg msg;
		size_t len;
		tFD fd = open("/dev/kmmanager",IO_READ | IO_WRITE);
		if(fd < 0)
			error("Unable to open keymap-manager");
		len = snprintf(path,sizeof(path),KEYMAP_DIR"/%s",kmname);
		if(sendMsgData(fd,MSG_KM_SET,path,len + 1) < 0 ||
				RETRY(receive(fd,NULL,&msg,sizeof(msg))) < 0 || (int)msg.args.arg1 < 0)
			fprintf(stderr,"Setting the keymap '%s' failed\n",kmname);
		else
			printf("Successfully changed keymap to '%s'\n",kmname);
		close(fd);
	}
	/* list all keymaps */
	else {
		sDirEntry e;
		DIR *dir = opendir(KEYMAP_DIR);
		if(!dir)
			error("Unable to open '%s'",KEYMAP_DIR);
		while(readdir(dir,&e)) {
			if(strcmp(e.name,".") != 0 && strcmp(e.name,"..") != 0)
				printf("%s\n",e.name);
		}
		closedir(dir);
	}
	return EXIT_SUCCESS;
}
