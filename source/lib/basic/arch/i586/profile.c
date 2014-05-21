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

#if defined(PROFILE)
#	include <assert.h>
#	include <esc/arch.h>
#	if defined(IN_KERNEL)
#		if 0
#			define gettid()		({ \
	uintptr_t __esp; \
	tid_t __tid; \
	GET_REG("sp",__esp); \
	if(proc_getByPid(1)) { \
		Thread *__t = Thread::getRunning(); \
		__tid = ((__esp >= KSTACK_AREA) && (uintptr_t)__t >= KERNEL_AREA) ? __t->tid : 0; \
	} \
	__tid; \
})
#		else
#			define gettid()		0
#		endif
#	define getcycles()	rdtsc()
#else
#	include <esc/thread.h>
#	include <esc/debug.h>
#	include <stdio.h>
#endif

#define STACK_SIZE	1024

static void logUnsigned(ullong n,uint base);
static void logChar(char c);

static uint64_t rdtsc(void) {
	uint32_t u, l;
	__asm__ volatile ("rdtsc" : "=a" (l), "=d" (u));
	return (uint64_t)u << 32 | l;
}

static uint8_t prof_inbyte(uint16_t port) {
	uint8_t res;
	__asm__ volatile ("in %1, %0" : "=a"(res) : "d"(port));
	return res;
}

static void prof_outbyte(uint16_t port,uint8_t val) {
	__asm__ volatile ("out %0, %1" : : "a"(val), "d"(port));
}

#if !IN_KERNEL
static bool initialized = false;
#else
int profEnabled = false;
#endif
static size_t stackPos = 0;
static bool inProf = false;
static uint64_t callStack[STACK_SIZE];

EXTERN_C void __cyg_profile_func_enter(void *this_fn,void *call_site);
EXTERN_C void __cyg_profile_func_exit(void *this_fn,void *call_site);

void __cyg_profile_func_enter(void *this_fn,A_UNUSED void *call_site) {
	if(inProf || !profEnabled)
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
	if(inProf || !profEnabled || stackPos <= 0)
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
	prof_outbyte(0xe9,c);
	prof_outbyte(0x3f8,c);
	while((prof_inbyte(0x3fd) & 0x20) == 0)
		;
}
#endif
