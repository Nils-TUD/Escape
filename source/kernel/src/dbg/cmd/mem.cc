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
#include <sys/dbg/cmd/mem.h>
#include <sys/dbg/kb.h>
#include <sys/mem/paging.h>
#include <sys/task/proc.h>
#include <sys/video.h>
#include <esc/keycodes.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

class MemNaviBackend : public NaviBackend {
public:
	explicit MemNaviBackend(Proc *p,uintptr_t addr)
		: NaviBackend(addr,ROUND_DN(ULONG_MAX,(ulong)BYTES_PER_LINE)),
		  proc(p), lastAddr(), page() {
	}
	virtual ~MemNaviBackend() {
		if(page)
			paging_removeAccess();
	}

	virtual const char *getInfo(uintptr_t) {
		static char procName[60];
		sStringBuffer buf;
		buf.dynamic = false;
		buf.len = 0;
		buf.size = sizeof(procName);
		buf.str = procName;
		prf_sprintf(&buf,"Process %d (%s)",proc->getPid(),proc->getCommand());
		return procName;
	}

	virtual uint8_t *loadLine(uintptr_t addr) {
		if(page == NULL || addr / PAGE_SIZE != lastAddr / PAGE_SIZE) {
			if(page)
				paging_removeAccess();
			if(paging_isPresent(proc->getPageDir(),addr)) {
				frameno_t frame = paging_getFrameNo(proc->getPageDir(),addr);
				page = (uint8_t*)paging_getAccess(frame);
			}
			else
				page = NULL;
			lastAddr = addr;
		}
		return page ? page + (addr & (PAGE_SIZE - 1)) : NULL;
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
	Proc *proc;
	uintptr_t lastAddr;
	uint8_t *page;
	static uint8_t buffer[BYTES_PER_LINE + 1];
};

static ScreenBackup backup;

int cons_cmd_mem(size_t argc,char **argv) {
	Proc *proc;
	uintptr_t addr = 0;
	if(Console::isHelp(argc,argv) || argc > 3) {
		vid_printf("Usage: %s [<pid> [<addr>]]\n",argv[0]);
		return 0;
	}

	if(argc > 1) {
		proc = Proc::getByPid(strtoul(argv[1],NULL,10));
		if(!proc) {
			vid_printf("The process with pid %d does not exist\n",strtoul(argv[1],NULL,10));
			return -ESRCH;
		}
		if(argc > 2)
			addr = (uintptr_t)strtoul(argv[2],NULL,16);
	}
	else
		proc = Proc::getByPid(Proc::getRunning());

	vid_backup(backup.screen,&backup.row,&backup.col);

	MemNaviBackend backend(proc,addr);
	Console::navigation(&backend);

	vid_restore(backup.screen,backup.row,backup.col);
	return 0;
}
