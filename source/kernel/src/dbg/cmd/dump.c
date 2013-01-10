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

static const char *getLineInfo(void *data,uintptr_t addr);
static uint8_t *loadLine(void *data,uintptr_t addr);
static bool lineMatches(void *data,uintptr_t addr,const char *search,size_t searchlen);
static void displayLine(void *data,uintptr_t addr,uint8_t *bytes);
static uintptr_t gotoAddr(void *data,const char *gotoAddr);

static sScreenBackup backup;
static uint8_t buffer[BYTES_PER_LINE + 1];
static pid_t pid;
static sFile *file = NULL;
static char filename[50];
static sNaviBackend backend;

int cons_cmd_dump(size_t argc,char **argv) {
	sStringBuffer buf;
	int res;
	pid = proc_getRunning();

	if(cons_isHelp(argc,argv) || argc != 2) {
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

		off_t end = vfs_seek(pid,file,0,SEEK_END);
		backend.startPos = 0;
		backend.maxPos = end & ~(BYTES_PER_LINE - 1);
		backend.loadLine = loadLine;
		backend.getInfo = getLineInfo;
		backend.lineMatches = lineMatches;
		backend.displayLine = displayLine;
		backend.gotoAddr = gotoAddr;
		cons_navigation(&backend,NULL);

		vid_restore(backup.screen,backup.row,backup.col);

		vfs_closeFile(pid,file);
	}
	return res;
}

static const char *getLineInfo(A_UNUSED void *data,A_UNUSED uintptr_t addr) {
	return filename;
}

static uint8_t *loadLine(A_UNUSED void *data,uintptr_t addr) {
	static uintptr_t lastAddr = -1;
	bool valid = true;
	if(lastAddr != addr) {
		memclear(buffer,sizeof(buffer));
		if(vfs_seek(pid,file,addr,SEEK_SET) < 0)
			valid = false;
		if(valid && vfs_readFile(pid,file,buffer,sizeof(buffer)) < 0)
			valid = false;
		if(valid)
			buffer[sizeof(buffer) - 1] = '\0';
		lastAddr = addr;
	}
	return valid ? buffer : NULL;
}

static bool lineMatches(void *data,uintptr_t addr,const char *search,size_t searchlen) {
	return cons_multiLineMatches(&backend,data,addr,search,searchlen);
}

static void displayLine(A_UNUSED void *data,uintptr_t addr,uint8_t *bytes) {
	cons_dumpLine(addr,bytes);
}

static uintptr_t gotoAddr(A_UNUSED void *data,const char *addr) {
	uintptr_t off = strtoul(addr,NULL,16);
	return off & ~((uintptr_t)BYTES_PER_LINE - 1);
}
