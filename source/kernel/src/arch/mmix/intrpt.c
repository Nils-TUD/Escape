/**
 * $Id: intrpt.c 906 2011-06-04 15:45:33Z nasmussen $
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <sys/task/signals.h>
#include <sys/task/timer.h>
#include <sys/task/uenv.h>
#include <sys/mem/paging.h>
#include <sys/mem/vmm.h>
#include <sys/dbg/kb.h>
#include <sys/dbg/console.h>
#include <esc/keycodes.h>
#include <sys/cpu.h>
#include <sys/syscalls.h>
#include <sys/intrpt.h>
#include <sys/video.h>
#include <sys/util.h>

static size_t irqCount = 0;
static sIntrptStackFrame *curFrame;

size_t intrpt_getCount(void) {
	return irqCount;
}

sIntrptStackFrame *intrpt_getCurStack(void) {
	return 0;
}

#if DEBUGGING

void intrpt_dbg_printStackFrame(const sIntrptStackFrame *stack) {
	int i;
	vid_printf("stack-frame @ 0x%Px\n",stack);
	vid_printf("\tirqNo=%d\n",stack->irqNo);
	vid_printf("\tpsw=#%08x\n",stack->psw);
	vid_printf("\tregister:\n");
	for(i = 0; i < REG_COUNT; i++)
		vid_printf("\tr[%d]=#%08x\n",i,stack->r[i]);
}

#endif
