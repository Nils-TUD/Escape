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

#define BYTES_PER_LINE	16
#define LINES			(VID_ROWS - 1)

static void displayMem(sProc *p,uintptr_t addr);

static sScreenBackup backup;
static char gotoAddr[9];
static size_t gotoPos = 0;

int cons_cmd_mem(size_t argc,char **argv) {
	sProc *p;
	sKeyEvent ev;
	uintptr_t addr = 0;
	bool run = true;
	if(argc < 2 || argc > 3) {
		vid_printf("Usage: %s <pid> [<addr>]\n",argv[0]);
		return -EINVAL;
	}

	p = proc_getByPid(strtoul(argv[1],NULL,10));
	if(!p) {
		vid_printf("The process with pid %d does not exist\n",strtoul(argv[1],NULL,10));
		return -ESRCH;
	}
	if(argc > 2)
		addr = (uintptr_t)strtoul(argv[2],NULL,16);

	vid_backup(backup.screen,&backup.row,&backup.col);
	while(run) {
		displayMem(p,addr);
		kb_get(&ev,KEV_PRESS,true);
		switch(ev.keycode) {
			case VK_UP:
				addr -= BYTES_PER_LINE;
				break;
			case VK_DOWN:
				addr += BYTES_PER_LINE;
				break;
			case VK_PGUP:
				addr -= BYTES_PER_LINE * LINES;
				break;
			case VK_PGDOWN:
				addr += BYTES_PER_LINE * LINES;
				break;
			case VK_0 ... VK_9:
			case VK_A:
			case VK_B:
			case VK_C:
			case VK_D:
			case VK_E:
			case VK_F:
				if(gotoPos < sizeof(gotoAddr) - 1) {
					gotoAddr[gotoPos++] = ev.character;
					gotoAddr[gotoPos] = '\0';
				}
				break;
			case VK_BACKSP:
				if(gotoPos > 0)
					gotoAddr[--gotoPos] = '\0';
				break;
			case VK_ENTER:
				if(gotoPos > 0) {
					addr = strtoul(gotoAddr,NULL,16);
					gotoPos = 0;
					gotoAddr[0] = '\0';
				}
				break;
			case VK_ESC:
			case VK_Q:
				run = false;
				break;
		}
	}
	vid_restore(backup.screen,backup.row,backup.col);
	return 0;
}

static void displayMem(sProc *p,uintptr_t addr) {
	char procName[50];
	sStringBuffer buf;
	uint y,i;
	uint8_t *page = NULL;
	uintptr_t lastAddr = 0;
	addr &= ~((uintptr_t)BYTES_PER_LINE - 1);
	vid_goto(0,0);
	for(y = 0; y < LINES; y++) {
		vid_printf("%p: ",addr);
		if(y == 0 || addr / PAGE_SIZE != lastAddr / PAGE_SIZE) {
			if(page)
				paging_removeAccess();
			if(paging_isPresent(&p->pagedir,addr)) {
				frameno_t frame = paging_getFrameNo(&p->pagedir,addr);
				page = (uint8_t*)paging_getAccess(frame);
			}
			else
				page = NULL;
			lastAddr = addr;
		}

		if(page) {
			uint8_t *bytes = page + (addr & (PAGE_SIZE - 1));
			for(i = 0; i < BYTES_PER_LINE; i++)
				vid_printf("%02X ",bytes[i]);
			vid_printf("| ");
			for(i = 0; i < BYTES_PER_LINE; i++) {
				if(isprint(bytes[i]) && bytes[i] < 0x80 && !isspace(bytes[i]))
					vid_printf("%c",bytes[i]);
				else
					vid_printf(".");
			}
		}
		else {
			for(i = 0; i < BYTES_PER_LINE; i++)
				vid_printf("?? ");
			vid_printf("| ");
			for(i = 0; i < BYTES_PER_LINE; i++)
				vid_printf(".");
		}
		vid_printf("\n");
		addr += BYTES_PER_LINE;
	}
	if(page)
		paging_removeAccess();

	buf.dynamic = false;
	buf.len = 0;
	buf.size = sizeof(procName);
	buf.str = procName;
	prf_sprintf(&buf,"Process %d (%s)",p->pid,p->command);
	vid_printf("\033[co;0;7]Goto: %s%|s\033[co]",gotoAddr,procName);
}
