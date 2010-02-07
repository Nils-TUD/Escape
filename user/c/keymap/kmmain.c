/**
 * $Id: kimain.c 312 2009-09-07 12:55:10Z nasmussen $
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
#include <esc/fileio.h>
#include <esc/cmdargs.h>
#include <esc/dir.h>
#include <stdlib.h>
#include <string.h>

#define KEYMAP_DIR		"/etc/keymaps"

int main(int argc,char **argv) {
	tFD dir;
	if(isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: %s [--set <name>]\n",argv[0]);
		return EXIT_FAILURE;
	}

	/* set keymap? */
	if(argc > 2 && strcmp(argv[1],"--set") == 0) {
		char path[MAX_PATH_LEN];
		u32 len;
		tFD fd = open("/drivers/kmmanager",IO_READ | IO_WRITE);
		if(fd < 0)
			error("Unable to open keymap-manager");
		len = snprintf(path,sizeof(path),KEYMAP_DIR"/%s",argv[2]);
		if(ioctl(fd,IOCTL_KM_SET,path,len + 1) < 0)
			fprintf(stderr,"Setting the keymap '%s' failed\n",argv[2]);
		else
			printf("Successfully changed keymap to '%s'\n",argv[2]);
		close(fd);
		return EXIT_SUCCESS;
	}

	/* list all keymaps */
	if((dir = opendir(KEYMAP_DIR)) >= 0) {
		sDirEntry e;
		while(readdir(&e,dir)) {
			if(strcmp(e.name,".") == 0 || strcmp(e.name,"..") == 0)
				continue;
			printf("%s\n",e.name);
		}
		closedir(dir);
	}

	return EXIT_SUCCESS;
}
