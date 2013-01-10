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

static const char *getLineInfo(void *data,uintptr_t addr);
static uint8_t *loadLine(void *data,uintptr_t addr);
static bool lineMatches(void *data,uintptr_t addr,const char *search,size_t searchlen);
static void displayLine(void *data,uintptr_t addr,uint8_t *bytes);
static uintptr_t gotoAddr(void *data,const char *gotoAddr);

static sScreenBackup backup;
static sProc *proc;
static uintptr_t lastAddr = 0;
static uint8_t *page = NULL;
static sNaviBackend backend;

int cons_cmd_mem(size_t argc,char **argv) {
	uintptr_t addr = 0;
	if(cons_isHelp(argc,argv) || argc > 3) {
		vid_printf("Usage: %s [<pid> [<addr>]]\n",argv[0]);
		return 0;
	}

	if(argc > 1) {
		proc = proc_getByPid(strtoul(argv[1],NULL,10));
		if(!proc) {
			vid_printf("The process with pid %d does not exist\n",strtoul(argv[1],NULL,10));
			return -ESRCH;
		}
		if(argc > 2)
			addr = (uintptr_t)strtoul(argv[2],NULL,16);
	}
	else
		proc = proc_getByPid(proc_getRunning());

	vid_backup(backup.screen,&backup.row,&backup.col);

	backend.startPos = addr;
	backend.maxPos = 0xFFFFFFFF & ~(BYTES_PER_LINE - 1);
	backend.loadLine = loadLine;
	backend.getInfo = getLineInfo;
	backend.lineMatches = lineMatches;
	backend.displayLine = displayLine;
	backend.gotoAddr = gotoAddr;
	cons_navigation(&backend,NULL);
	if(page)
		paging_removeAccess();

	vid_restore(backup.screen,backup.row,backup.col);
	return 0;
}

static const char *getLineInfo(A_UNUSED void *data,A_UNUSED uintptr_t addr) {
	static char procName[60];
	sStringBuffer buf;
	buf.dynamic = false;
	buf.len = 0;
	buf.size = sizeof(procName);
	buf.str = procName;
	prf_sprintf(&buf,"Process %d (%s)",proc->pid,proc->command);
	return procName;
}

static uint8_t *loadLine(A_UNUSED void *data,uintptr_t addr) {
	if(page == NULL || addr / PAGE_SIZE != lastAddr / PAGE_SIZE) {
		if(page)
			paging_removeAccess();
		if(paging_isPresent(&proc->pagedir,addr)) {
			frameno_t frame = paging_getFrameNo(&proc->pagedir,addr);
			page = (uint8_t*)paging_getAccess(frame);
		}
		else
			page = NULL;
		lastAddr = addr;
	}
	return page ? page + (addr & (PAGE_SIZE - 1)) : NULL;
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
