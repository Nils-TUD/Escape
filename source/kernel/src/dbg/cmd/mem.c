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

static void displayMem(const char *gotoAddr,uintptr_t addr);

static sScreenBackup backup;
static sProc *proc;

int cons_cmd_mem(size_t argc,char **argv) {
	uintptr_t addr = 0;
	if(argc < 2 || argc > 3) {
		vid_printf("Usage: %s <pid> [<addr>]\n",argv[0]);
		return -EINVAL;
	}

	proc = proc_getByPid(strtoul(argv[1],NULL,10));
	if(!proc) {
		vid_printf("The process with pid %d does not exist\n",strtoul(argv[1],NULL,10));
		return -ESRCH;
	}
	if(argc > 2)
		addr = (uintptr_t)strtoul(argv[2],NULL,16);

	vid_backup(backup.screen,&backup.row,&backup.col);
	cons_navigation(addr,displayMem);
	vid_restore(backup.screen,backup.row,backup.col);
	return 0;
}

static void displayMem(const char *gotoAddr,uintptr_t addr) {
	sStringBuffer buf;
	char procName[60];
	uint y;
	uint8_t *page = NULL;
	uintptr_t lastAddr = 0;
	vid_goto(0,0);
	for(y = 0; y < VID_ROWS - 1; y++) {
		if(y == 0 || addr / PAGE_SIZE != lastAddr / PAGE_SIZE) {
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

		cons_dumpLine(addr,page ? page + (addr & (PAGE_SIZE - 1)) : NULL);
		addr += BYTES_PER_LINE;
	}
	if(page)
		paging_removeAccess();

	buf.dynamic = false;
	buf.len = 0;
	buf.size = sizeof(procName);
	buf.str = procName;
	prf_sprintf(&buf,"Process %d (%s)",proc->pid,proc->command);
	vid_printf("\033[co;0;7]Goto: %s%|s\033[co]",gotoAddr,procName);
}
