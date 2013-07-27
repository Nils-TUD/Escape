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
#include <sys/task/proc.h>
#include <sys/mem/kheap.h>
#include <sys/vfs/vfs.h>
#include <sys/video.h>
#include <esc/keycodes.h>
#include <errno.h>
#include <string.h>

static ScreenBackup backup;
static sFile *file;
static pid_t pid;

class DumpNaviBackend : public NaviBackend {
public:
	explicit DumpNaviBackend(const char *f,uintptr_t end)
			: NaviBackend(0,end), pid(Proc::getRunning()) {
		sStringBuffer buf;
		buf.dynamic = false;
		buf.len = 0;
		buf.size = sizeof(filename);
		buf.str = filename;
		prf_sprintf(&buf,"File %s",f);
	}

	virtual const char *getInfo(uintptr_t) {
		return filename;
	}

	virtual uint8_t *loadLine(uintptr_t addr) {
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

	virtual bool lineMatches(uintptr_t addr,const char *search,size_t searchlen) {
		return Console::multiLineMatches(this,addr,search,searchlen);
	}

	virtual void displayLine(uintptr_t addr,uint8_t *bytes) {
		Console::dumpLine(addr,bytes);
	}

	virtual uintptr_t gotoAddr(const char *addr) {
		uintptr_t off = strtoul(addr,NULL,16);
		return ROUND_DN(off,(uintptr_t)BYTES_PER_LINE);
	}

private:
	pid_t pid;
	static uint8_t buffer[BYTES_PER_LINE + 1];
	static char filename[50];
};

uint8_t DumpNaviBackend::buffer[BYTES_PER_LINE + 1];
char DumpNaviBackend::filename[50];

int cons_cmd_dump(size_t argc,char **argv) {
	int res;

	if(Console::isHelp(argc,argv) || argc != 2) {
		vid_printf("Usage: %s <file>\n",argv[0]);
		vid_printf("\tUses the current proc to be able to access the real-fs.\n");
		vid_printf("\tSo, I hope, you know what you're doing ;)\n");
		return 0;
	}

	pid = Proc::getRunning();
	res = vfs_openPath(pid,VFS_READ,argv[1],&file);
	if(res >= 0) {
		vid_backup(backup.screen,&backup.row,&backup.col);

		off_t end = vfs_seek(pid,file,0,SEEK_END);
		DumpNaviBackend backend(argv[1],ROUND_DN(end,(uintptr_t)BYTES_PER_LINE));
		Console::navigation(&backend);

		vid_restore(backup.screen,backup.row,backup.col);

		vfs_closeFile(pid,file);
	}
	return res;
}
