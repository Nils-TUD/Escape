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
#include <sys/dbg/cmd/dump.h>
#include <sys/task/thread.h>
#include <sys/mem/kheap.h>
#include <sys/vfs/vfs.h>
#include <sys/video.h>
#include <esc/keycodes.h>
#include <errno.h>
#include <string.h>

static void displayContent(const char *gotoAddr,uintptr_t offset);

static sScreenBackup backup;
static uint8_t buffer[(VID_ROWS - 1) * BYTES_PER_LINE];
static pid_t pid;
static sFile *file = NULL;
static char filename[50];

int cons_cmd_dump(size_t argc,char **argv) {
	sStringBuffer buf;
	int res;
	pid = proc_getRunning();

	if(argc != 2) {
		vid_printf("Usage: %s <file>\n",argv[0]);
		vid_printf("\tUses the current proc to be able to access the real-fs.\n");
		vid_printf("\tSo, I hope, you know what you're doing ;)\n");
		return 0;
	}

	buf.dynamic = false;
	buf.len = 0;
	buf.size = sizeof(filename);
	buf.str = filename;
	prf_sprintf(&buf,"File %s",argv[1]);

	res = vfs_openPath(pid,VFS_READ,argv[1],&file);
	if(res >= 0) {
		vid_backup(backup.screen,&backup.row,&backup.col);
		cons_navigation(0,displayContent);
		vid_restore(backup.screen,backup.row,backup.col);

		vfs_closeFile(pid,file);
	}
	return res;
}

static void displayContent(const char *gotoAddr,uintptr_t offset) {
	memclear(buffer,sizeof(buffer));

	bool valid = true;
	if(vfs_seek(pid,file,offset,SEEK_SET) < 0)
		valid = false;
	if(valid && vfs_readFile(pid,file,buffer,sizeof(buffer)) < 0)
		valid = false;

	uint y;
	uint8_t *bytes = buffer;
	vid_goto(0,0);
	for(y = 0; y < VID_ROWS - 1; y++) {
		cons_dumpLine(offset,valid ? bytes : NULL);
		bytes += BYTES_PER_LINE;
		offset += BYTES_PER_LINE;
	}
	vid_printf("\033[co;0;7]Goto: %s%|s\033[co]",gotoAddr,filename);
}
