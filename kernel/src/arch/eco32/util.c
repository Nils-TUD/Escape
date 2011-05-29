/**
 * $Id: util.c 847 2010-10-04 01:25:15Z nasmussen $
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
#include <sys/ksymbols.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/log.h>
#include <esc/keycodes.h>
#include <stdarg.h>
#include <string.h>

void util_panic(const char *fmt,...) {
	/*sIntrptStackFrame *istack = intrpt_getCurStack();
	sThread *t = thread_getRunning();*/
	va_list ap;

	/* disable interrupts so that nothing fancy can happen */
	/*intrpt_setEnabled(false);*/

	/* print message */
	vid_setTargets(TARGET_SCREEN | TARGET_LOG);
	vid_printf("\n");
	vid_printf("\033[co;7;4]PANIC: ");
	va_start(ap,fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);
	vid_printf("%|s\033[co]\n","");

	/*if(t != NULL)
		vid_printf("Caused by thread %d (%s)\n\n",t->tid,t->proc->command);
	util_printStackTrace(util_getKernelStackTrace());

	if(t != NULL && t->stackRegion) {
		util_printStackTrace(util_getUserStackTrace());
		vid_printf("User-Register:\n");
		regs[R_EAX] = istack->eax;
		regs[R_EBX] = istack->ebx;
		regs[R_ECX] = istack->ecx;
		regs[R_EDX] = istack->edx;
		regs[R_ESI] = istack->esi;
		regs[R_EDI] = istack->edi;
		regs[R_ESP] = istack->uesp;
		regs[R_EBP] = istack->ebp;
		regs[R_CS] = istack->cs;
		regs[R_DS] = istack->ds;
		regs[R_ES] = istack->es;
		regs[R_FS] = istack->fs;
		regs[R_GS] = istack->gs;
		regs[R_SS] = istack->uss;
		regs[R_EFLAGS] = istack->eflags;
		PRINT_REGS(regs,"\t");
	}*/

#if DEBUGGING
	/* write into log only */
	/*vid_setTargets(TARGET_SCREEN);
	vid_printf("\n\nWriting regions and page-directory of the current process to log...");
	vid_setTargets(TARGET_LOG);
	vmm_dbg_print(t->proc);
	paging_dbg_printCur(PD_PART_USER);
	vid_setTargets(TARGET_SCREEN);
	vid_printf("Done\n\nPress any key to start debugger");
	while(1) {
		kb_get(NULL,KEV_PRESS,true);
		cons_start();
	}*/

	while(1);

#else
	/* TODO vmware seems to shutdown if we disable interrupts and htl?? */
	while(1)
		util_halt();
#endif
}

void util_dumpMem(const void *addr,size_t wordCount) {
	uint *ptr = (uint*)addr;
	while(wordCount-- > 0) {
		vid_printf("0x%x: 0x%08x\n",ptr,*ptr);
		ptr++;
	}
}

void util_dumpBytes(const void *addr,size_t byteCount) {
	size_t i = 0;
	uchar *ptr = (uchar*)addr;
	for(i = 0; byteCount-- > 0; i++) {
		vid_printf("%02x ",*ptr);
		ptr++;
		if(i % 16 == 15)
			vid_printf("\n");
	}
}

