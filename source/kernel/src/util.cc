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
#include <sys/task/proc.h>
#include <sys/mem/pmem.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/virtmem.h>
#include <sys/dbg/kb.h>
#include <sys/dbg/console.h>
#include <sys/intrpt.h>
#include <sys/ksymbols.h>
#include <sys/video.h>
#include <sys/spinlock.h>
#include <sys/util.h>
#include <sys/log.h>
#include <stdarg.h>
#include <string.h>

/* source: http://en.wikipedia.org/wiki/Linear_congruential_generator */
static uint randa = 1103515245;
static uint randc = 12345;
static uint lastRand = 0;
static klock_t randLock;

int util_rand(void) {
	int res;
	spinlock_aquire(&randLock);
	lastRand = randa * lastRand + randc;
	res = (int)((uint)(lastRand / 65536) % 32768);
	spinlock_release(&randLock);
	return res;
}

void util_srand(uint seed) {
	spinlock_aquire(&randLock);
	lastRand = seed;
	spinlock_release(&randLock);
}

void util_panic(const char *fmt, ...) {
	va_list ap;
	const Thread *t = Thread::getRunning();
	util_panic_arch();
	vid_clearScreen();

	/* print message */
	vid_setTargets(TARGET_SCREEN | TARGET_LOG);
	vid_printf("\n");
	vid_printf("\033[co;7;4]PANIC: ");
	va_start(ap,fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);
	vid_printf("%|s\033[co]\n","");

	/* write information about the running thread to log/screen */
	if(t != NULL)
		vid_printf("Caused by thread %d (%s)\n\n",t->getTid(),t->getProc()->getCommand());
	util_printStackTrace(util_getKernelStackTrace());
	if(t) {
		util_printUserState();

		vid_setTargets(TARGET_LOG);
		vid_printf("\n============= snip =============\n");
		vid_printf("Region overview:\n");
		t->getProc()->getVM()->printShort("\t");

		vid_setTargets(TARGET_SCREEN | TARGET_LOG);
		util_printStackTrace(util_getUserStackTrace());

		vid_setTargets(TARGET_LOG);
		vid_printf("============= snip =============\n\n");
		vid_setTargets(TARGET_SCREEN | TARGET_LOG);
	}

	/* write into log only */
	if(t) {
		vid_setTargets(TARGET_SCREEN);
		vid_printf("\n\nWriting regions and page-directory of the current process to log...");
		vid_setTargets(TARGET_LOG);
		t->getProc()->getVM()->printRegions();
		paging_printCur(PD_PART_USER);
		vid_setTargets(TARGET_SCREEN);
		vid_printf("Done\n");
	}

	vid_printf("\nPress any key to start debugger");
	while(1) {
		kb_get(NULL,KEV_PRESS,true);
		cons_start(NULL);
	}
}

void util_printEventTrace(const sFuncCall *trace,const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);
	if(trace) {
		size_t i;
		for(i = 0; trace->addr != 0 && i < 5; i++) {
			sSymbol *sym = ksym_getSymbolAt(trace->addr);
			if(sym->address)
				vid_printf("%s",sym->funcName);
			else
				vid_printf("%Px",trace->addr);
			trace++;
			if(trace->addr)
				vid_printf(" ");
		}
	}
	vid_printf("\n");
}

void util_printStackTrace(const sFuncCall *trace) {
	if(trace && trace->addr) {
		if(trace->addr < KERNEL_AREA)
			vid_printf("User-Stacktrace:\n");
		else
			vid_printf("Kernel-Stacktrace:\n");

		while(trace->addr != 0) {
			vid_printf("\t%p -> %p (%s)\n",(trace + 1)->addr,trace->funcAddr,trace->funcName);
			trace++;
		}
	}
}

void util_dumpMem(const void *addr,size_t dwordCount) {
	ulong *ptr = (ulong*)addr;
	while(dwordCount-- > 0) {
		vid_printf("%p: 0x%0*lx\n",ptr,sizeof(ulong) * 2,*ptr);
		ptr++;
	}
}

void util_dumpBytes(const void *addr,size_t byteCount) {
	size_t i = 0;
	uchar *ptr = (uchar*)addr;
	for(i = 0; byteCount-- > 0; i++) {
		if(i % 16 == 0) {
			if(i > 0)
				vid_printf("\n");
			vid_printf("%p:",ptr);
		}
		vid_printf(" %02x",*ptr);
		ptr++;
	}
}
