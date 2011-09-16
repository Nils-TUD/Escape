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
#include <sys/mem/vmm.h>
#include <sys/mem/cache.h>
#include <sys/debug.h>
#include <sys/intrpt.h>
#include <sys/util.h>
#include <sys/cpu.h>
#include <sys/video.h>
#include <stdarg.h>
#include <string.h>

static sFuncCall frames[1] = {
	{0,0,""}
};

void util_panic(const char *fmt,...) {
	sKSpecRegs *sregs = thread_getSpecRegs();
	sThread *t = thread_getRunning();
	va_list ap;

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
	intrpt_printStackFrame(thread_getIntrptStack(t));
	vid_printf("\trBB : #%016lx rWW : #%016lx rXX : #%016lx\n",sregs->rbb,sregs->rww,sregs->rxx);
	vid_printf("\trYY : #%016lx rZZ : #%016lx\n",sregs->ryy,sregs->rzz);

	/* write into log only */
	vid_setTargets(TARGET_SCREEN);
	vid_printf("\n\nWriting regions and page-directory of the current process to log...");
	vid_setTargets(TARGET_LOG);
	vmm_print(t->proc->pid);
	paging_printCur(PD_PART_USER);
	cache_print();
	vid_setTargets(TARGET_SCREEN);
	vid_printf("Done\n\nPress any key to start debugger");
	while(1) {
		kb_get(NULL,KEV_PRESS,true);
		cons_start();
	}
}

sFuncCall *util_getUserStackTrace(void) {
	/* TODO */
	/* the MMIX-toolchain doesn't use a frame-pointer when enabling optimization, as it seems.
	 * therefore without information about the stackframe-sizes or similar, there is no way to
	 * determine the stacktrace */
	return frames;
}

sFuncCall *util_getKernelStackTrace(void) {
	return frames;
}

sFuncCall *util_getUserStackTraceOf(A_UNUSED sThread *t) {
	return frames;
}

sFuncCall *util_getKernelStackTraceOf(A_UNUSED const sThread *t) {
	return frames;
}
