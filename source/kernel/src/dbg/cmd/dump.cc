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

#include <dbg/cmd/dump.h>
#include <dbg/console.h>
#include <dbg/kb.h>
#include <dbg/lines.h>
#include <mem/kheap.h>
#include <sys/keycodes.h>
#include <task/proc.h>
#include <task/thread.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <common.h>
#include <errno.h>
#include <ostringstream.h>
#include <string.h>

static OpenFile *file;
static pid_t pid;

class DumpNaviBackend : public NaviBackend {
public:
	explicit DumpNaviBackend(const char *f,uintptr_t end)
			: NaviBackend(0,end), pid(Proc::getRunning()) {
		OStringStream os(filename,sizeof(filename));
		os.writef("File %s",f);
	}

	virtual const char *getInfo(uintptr_t) {
		return filename;
	}

	virtual uint8_t *loadLine(uintptr_t addr) {
		static uintptr_t lastAddr = -1;
		bool valid = true;
		if(lastAddr != addr) {
			memclear(buffer,sizeof(buffer));
			if(file->seek(pid,addr,SEEK_SET) < 0)
				valid = false;
			if(valid && file->read(pid,buffer,sizeof(buffer)) < 0)
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

	virtual void displayLine(OStream &os,uintptr_t addr,uint8_t *bytes) {
		Console::dumpLine(os,addr,bytes);
	}

	virtual uintptr_t gotoAddr(const char *addr) {
		uintptr_t off = strtoul(addr,NULL,16);
		return ROUND_DN(off,(uintptr_t)Console::BYTES_PER_LINE);
	}

private:
	pid_t pid;
	static uint8_t buffer[Console::BYTES_PER_LINE + 1];
	static char filename[50];
};

uint8_t DumpNaviBackend::buffer[Console::BYTES_PER_LINE + 1];
char DumpNaviBackend::filename[50];

int cons_cmd_dump(OStream &os,size_t argc,char **argv) {
	if(Console::isHelp(argc,argv) || argc != 2) {
		os.writef("Usage: %s <file>\n",argv[0]);
		os.writef("\tUses the current proc to be able to access the real-fs.\n");
		os.writef("\tSo, I hope, you know what you're doing ;)\n");
		return 0;
	}

	pid = Proc::getRunning();
	int res = VFS::openPath(pid,VFS_READ,0,argv[1],&file);
	if(res >= 0) {
		off_t end = file->seek(pid,0,SEEK_END);
		DumpNaviBackend backend(argv[1],ROUND_DN(end,(uintptr_t)Console::BYTES_PER_LINE));
		Console::navigation(os,&backend);
		file->close(pid);
	}
	return res;
}
