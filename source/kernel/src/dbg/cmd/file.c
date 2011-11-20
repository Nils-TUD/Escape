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

#include <sys/common.h>
#include <sys/dbg/console.h>
#include <sys/dbg/kb.h>
#include <sys/dbg/lines.h>
#include <sys/dbg/cmd/file.h>
#include <sys/task/thread.h>
#include <sys/mem/kheap.h>
#include <sys/vfs/vfs.h>
#include <sys/video.h>
#include <esc/keycodes.h>
#include <errno.h>

static sScreenBackup backup;
static char buffer[512];

int cons_cmd_file(size_t argc,char **argv) {
	pid_t pid = proc_getRunning();
	sFile *file = NULL;
	ssize_t i,count;
	int res;
	sLines lines;

	if(argc != 2) {
		vid_printf("Usage: %s <file>\n",argv[0]);
		vid_printf("\tUses the current proc to be able to access the real-fs.\n");
		vid_printf("\tSo, I hope, you know what you're doing ;)\n");
		return 0;
	}

	vid_backup(backup.screen,&backup.row,&backup.col);

	if((res = lines_create(&lines)) < 0)
		goto error;

	res = vfs_openPath(pid,VFS_READ,argv[1],&file);
	if(res < 0)
		goto error;
	while((count = vfs_readFile(pid,file,buffer,sizeof(buffer))) > 0) {
		/* build lines from the read data */
		for(i = 0; i < count; i++) {
			if(buffer[i] == '\n')
				lines_newline(&lines);
			else
				lines_append(&lines,buffer[i]);
		}
	}
	lines_end(&lines);
	vfs_closeFile(pid,file);
	file = NULL;

	/* now display lines */
	cons_viewLines(&lines);
	res = 0;

error:
	/* clean up */
	lines_destroy(&lines);
	if(file != NULL)
		vfs_closeFile(pid,file);

	vid_restore(backup.screen,backup.row,backup.col);
	return res;
}
