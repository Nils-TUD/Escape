/**
 * $Id$
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
#include <sys/mem/paging.h>
#include <sys/mem/vmm.h>
#include <sys/dbg/console.h>
#include <sys/dbg/kb.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/intrpt.h>
#include <sys/ksymbols.h>
#include <sys/video.h>
#include <sys/util.h>
#include <stdarg.h>
#include <string.h>

static sFuncCall frames[1] = {
	{0,0,""}
};

void util_panic(const char *fmt,...) {
	sThread *t = thread_getRunning();
	sIntrptStackFrame *istack = t->kstackEnd;
	va_list ap;
	int i;

	/* print message */
	vid_setTargets(TARGET_SCREEN | TARGET_LOG);
	vid_printf("\n");
	vid_printf("\033[co;7;4]PANIC: ");
	va_start(ap,fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);
	vid_printf("%|s\033[co]\n","");

	if(t != NULL)
		vid_printf("Caused by thread %d (%s)\n\n",t->tid,t->proc->command);

	vid_printf("User state:\n");
	vid_printf("\tPSW: 0x%08x\n\t",istack->psw);
	for(i = 0; i < REG_COUNT; i++) {
		int row = i / 4;
		int col = i % 4;
		vid_printf("$%-2d: 0x%08x ",col * 8 + row,istack->r[col * 8 + row]);
		if(i % 4 == 3)
			vid_printf("\n\t");
	}

#if DEBUGGING
	/* write into log only */
	vid_setTargets(TARGET_SCREEN);
	vid_printf("\n\nWriting regions and page-directory of the current process to log...");
	vid_setTargets(TARGET_LOG);
	vmm_dbg_print(t->proc);
	paging_dbg_printCur(PD_PART_USER);
	vid_setTargets(TARGET_SCREEN);
	vid_printf("Done\n\nPress any key to start debugger");
	while(1) {
		kb_get(NULL,KEV_PRESS,true);
		cons_start();
	}
#else
	while(1);
#endif
}

void util_copyToUser(void *dst,const void *src,size_t count) {
	/* on eco32 we can't write to non-writable pages in kernel-mode */
	/* therefore, we copy it page by page and use the direct mapped space */
	sProc *p = proc_getRunning();
	uintptr_t isrc = (uintptr_t)src;
	uintptr_t idst = (uintptr_t)dst;
	size_t offset = idst & (PAGE_SIZE - 1);
	while(count > 0) {
		tFrameNo frameNo = paging_getFrameNo(p->pagedir,idst);
		size_t amount = MIN(count,PAGE_SIZE - offset);
		memcpy((void*)((frameNo * PAGE_SIZE + offset) | DIR_MAPPED_SPACE),(void*)isrc,amount);
		idst += amount;
		isrc += amount;
		count -= amount;
		offset = 0;
	}
}

void util_zeroToUser(void *dst,size_t count) {
	sProc *p = proc_getRunning();
	uintptr_t idst = (uintptr_t)dst;
	size_t offset = idst & (PAGE_SIZE - 1);
	while(count > 0) {
		tFrameNo frameNo = paging_getFrameNo(p->pagedir,idst);
		size_t amount = MIN(count,PAGE_SIZE - offset);
		memclear((void*)((frameNo * PAGE_SIZE + offset) | DIR_MAPPED_SPACE),amount);
		idst += amount;
		count -= amount;
		offset = 0;
	}
}

sFuncCall *util_getUserStackTrace(void) {
	/* eco32 has no frame-pointer; therefore without information about the stackframe-sizes or
	 * similar, there is no way to determine the stacktrace */
	return frames;
}

sFuncCall *util_getKernelStackTrace(void) {
	return frames;
}

sFuncCall *util_getUserStackTraceOf(const sThread *t) {
	UNUSED(t);
	return frames;
}

sFuncCall *util_getKernelStackTraceOf(const sThread *t) {
	UNUSED(t);
	return frames;
}

