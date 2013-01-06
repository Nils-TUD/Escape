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

#include <assert.h>
#ifdef IN_KERNEL
#	include <esc/arch/i586/register.h>
#	include <sys/arch/i586/ports.h>
#	include <sys/task/thread.h>
#	include <sys/mem/paging.h>
#	include <sys/ksymbols.h>
#	include <sys/cpu.h>
#	include <sys/video.h>
#	include <sys/util.h>
#	define outb			ports_outByte
#	define inb			ports_inByte
#	if 1
#		define gettid()		({ \
	uintptr_t __esp; \
	tid_t __tid; \
	GET_REG("esp",__esp); \
	if(proc_getByPid(1)) { \
		sThread *__t = thread_getRunning(); \
		__tid = ((__esp >= KERNEL_STACK_AREA) && (uintptr_t)__t >= KERNEL_AREA) ? __t->tid : 0; \
	} \
	__tid; \
})
#	else
#		define gettid()		0
#	endif
#	define getcycles()	cpu_rdtsc()
#else
#	include <esc/arch/i586/ports.h>
#	include <esc/thread.h>
#	include <esc/debug.h>
#	include <stdio.h>
#	define outb			outbyte
#	define inb			inbyte
#endif

#define STACK_SIZE	1024

#ifdef PROFILE
static void logUnsigned(ullong n,uint base);
static void logChar(char c);

#if !IN_KERNEL
static bool initialized = false;
#endif
static size_t stackPos = 0;
static bool inProf = false;
static uint64_t callStack[STACK_SIZE];

#ifdef __cplusplus
extern "C" {
#endif

void __cyg_profile_func_enter(void *this_fn,void *call_site);
void __cyg_profile_func_exit(void *this_fn,void *call_site);

#ifdef __cplusplus
}
#endif

void __cyg_profile_func_enter(void *this_fn,A_UNUSED void *call_site) {
	if(inProf)
		return;
	inProf = true;
#if !IN_KERNEL
	if(!initialized) {
		reqport(0xe9);
		reqport(0x3f8);
		reqport(0x3fd);
		initialized = true;
	}
#endif
	logChar('>');
	logUnsigned((unsigned)gettid(),10);
	logChar(':');
	logUnsigned((unsigned)this_fn,16);
	logChar('\n');
	callStack[stackPos++] = getcycles();
	inProf = false;
}

void __cyg_profile_func_exit(A_UNUSED void *this_fn,A_UNUSED void *call_site) {
	uint64_t now;
	if(inProf || stackPos <= 0)
		return;
	inProf = true;
	now = getcycles();
	stackPos--;
	logChar('<');
	logUnsigned((unsigned)gettid(),10);
	logChar(':');
	logUnsigned(now - callStack[stackPos],10);
	logChar('\n');
	inProf = false;
}

static void logUnsigned(ullong n,uint base) {
	if(n >= base)
		logUnsigned(n / base,base);
	logChar("0123456789ABCDEF"[(n % base)]);
}

static void logChar(char c) {
	outb(0xe9,c);
	outb(0x3f8,c);
	while((inb(0x3fd) & 0x20) == 0)
		;
}
#endif
