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

#include <common.h>
#include <dbg/console.h>
#include <dbg/cmd/mem.h>
#include <dbg/kb.h>
#include <mem/pagedir.h>
#include <task/proc.h>
#include <ostringstream.h>
#include <esc/keycodes.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

class MemNaviBackend : public NaviBackend {
public:
	explicit MemNaviBackend(Proc *p,uintptr_t addr)
		: NaviBackend(addr,ROUND_DN(ULONG_MAX,(ulong)BYTES_PER_LINE)),
		  proc(p), lastAddr(), lastFrame(), page() {
	}
	virtual ~MemNaviBackend() {
		if(page)
			PageDir::removeAccess(lastFrame);
	}

	virtual const char *getInfo(uintptr_t) {
		static char procName[60];
		OStringStream os(procName,sizeof(procName));
		os.writef("Process %d (%s)",proc->getPid(),proc->getProgram());
		return procName;
	}

	virtual uint8_t *loadLine(uintptr_t addr) {
		if(page == NULL || addr / PAGE_SIZE != lastAddr / PAGE_SIZE) {
			if(page)
				PageDir::removeAccess(lastFrame);
			if(proc->getPageDir()->isPresent(addr)) {
				lastFrame = proc->getPageDir()->getFrameNo(addr);
				page = (uint8_t*)PageDir::getAccess(lastFrame);
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

	virtual void displayLine(OStream &os,uintptr_t addr,uint8_t *bytes) {
		Console::dumpLine(os,addr,bytes);
	}

	virtual uintptr_t gotoAddr(const char *addr) {
		uintptr_t off = strtoul(addr,NULL,16);
		return ROUND_DN(off,(uintptr_t)BYTES_PER_LINE);
	}

private:
	Proc *proc;
	uintptr_t lastAddr;
	frameno_t lastFrame;
	uint8_t *page;
	static uint8_t buffer[BYTES_PER_LINE + 1];
};

int cons_cmd_mem(OStream &os,size_t argc,char **argv) {
	Proc *proc;
	uintptr_t addr = 0;
	if(Console::isHelp(argc,argv) || argc > 3) {
		os.writef("Usage: %s [<pid> [<addr>]]\n",argv[0]);
		return 0;
	}

	if(argc > 1) {
		proc = Proc::getRef(strtoul(argv[1],NULL,10));
		if(!proc) {
			os.writef("The process with pid %d does not exist\n",strtoul(argv[1],NULL,10));
			return -ESRCH;
		}
		if(argc > 2)
			addr = (uintptr_t)strtoul(argv[2],NULL,16);
	}
	else
		proc = Proc::getRef(Proc::getRunning());

	MemNaviBackend backend(proc,addr);
	Proc::relRef(proc);

	Console::navigation(os,&backend);
	return 0;
}
