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
#include <sys/mem/physmem.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/virtmem.h>
#include <sys/dbg/kb.h>
#include <sys/dbg/console.h>
#include <sys/interrupts.h>
#include <sys/ksymbols.h>
#include <sys/video.h>
#include <sys/spinlock.h>
#include <sys/util.h>
#include <sys/cpu.h>
#include <sys/log.h>
#include <stdarg.h>
#include <string.h>

/* source: http://en.wikipedia.org/wiki/Linear_congruential_generator */
uint Util::randa = 1103515245;
uint Util::randc = 12345;
uint Util::lastRand = 0;
klock_t Util::randLock;
uint64_t Util::profStart;

int Util::rand() {
	int res;
	SpinLock::aquire(&randLock);
	lastRand = randa * lastRand + randc;
	res = (int)((uint)(lastRand / 65536) % 32768);
	SpinLock::release(&randLock);
	return res;
}

void Util::srand(uint seed) {
	SpinLock::aquire(&randLock);
	lastRand = seed;
	SpinLock::release(&randLock);
}

void Util::startTimer() {
	profStart = CPU::rdtsc();
}

void Util::stopTimer(const char *prefix,...) {
	va_list l;
	uLongLong diff;
	diff.val64 = CPU::rdtsc() - profStart;
	va_start(l,prefix);
	Video::vprintf(prefix,l);
	va_end(l);
	Video::printf(": 0x%08x%08x\n",diff.val32.upper,diff.val32.lower);
}

void Util::panic(const char *fmt, ...) {
	va_list ap;
	const Thread *t = Thread::getRunning();
	panicArch();
	Video::clearScreen();

	/* print message */
	Video::setTargets(Video::SCREEN | Video::LOG);
	Video::printf("\n");
	Video::printf("\033[co;7;4]PANIC: ");
	va_start(ap,fmt);
	Video::vprintf(fmt,ap);
	va_end(ap);
	Video::printf("%|s\033[co]\n","");

	/* write information about the running thread to log/screen */
	if(t != NULL)
		Video::printf("Caused by thread %d (%s)\n\n",t->getTid(),t->getProc()->getCommand());
	printStackTrace(getKernelStackTrace());
	if(t) {
		printUserState();

		Video::setTargets(Video::LOG);
		Video::printf("\n============= snip =============\n");
		Video::printf("Region overview:\n");
		t->getProc()->getVM()->printShort("\t");

		Video::setTargets(Video::SCREEN | Video::LOG);
		printStackTrace(getUserStackTrace());

		Video::setTargets(Video::LOG);
		Video::printf("============= snip =============\n\n");
		Video::setTargets(Video::SCREEN | Video::LOG);
	}

	/* write into log only */
	if(t) {
		Video::setTargets(Video::SCREEN);
		Video::printf("\n\nWriting regions and page-directory of the current process to log...");
		Video::setTargets(Video::LOG);
		t->getProc()->getVM()->printRegions();
		t->getProc()->getPageDir()->print(PD_PART_USER);
		Video::setTargets(Video::SCREEN);
		Video::printf("Done\n");
	}

	Video::printf("\nPress any key to start debugger");
	while(1) {
		Keyboard::get(NULL,KEV_PRESS,true);
		Console::start(NULL);
	}
}

void Util::printEventTrace(const FuncCall *trace,const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	Video::vprintf(fmt,ap);
	va_end(ap);
	if(trace) {
		size_t i;
		for(i = 0; trace->addr != 0 && i < 5; i++) {
			sSymbol *sym = ksym_getSymbolAt(trace->addr);
			if(sym->address)
				Video::printf("%s",sym->funcName);
			else
				Video::printf("%Px",trace->addr);
			trace++;
			if(trace->addr)
				Video::printf(" ");
		}
	}
	Video::printf("\n");
}

void Util::printStackTrace(const FuncCall *trace) {
	if(trace && trace->addr) {
		if(trace->addr < KERNEL_AREA)
			Video::printf("User-Stacktrace:\n");
		else
			Video::printf("Kernel-Stacktrace:\n");

		while(trace->addr != 0) {
			Video::printf("\t%p -> %p (%s)\n",(trace + 1)->addr,trace->funcAddr,trace->funcName);
			trace++;
		}
	}
}

void Util::dumpMem(const void *addr,size_t dwordCount) {
	ulong *ptr = (ulong*)addr;
	while(dwordCount-- > 0) {
		Video::printf("%p: 0x%0*lx\n",ptr,sizeof(ulong) * 2,*ptr);
		ptr++;
	}
}

void Util::dumpBytes(const void *addr,size_t byteCount) {
	size_t i = 0;
	uchar *ptr = (uchar*)addr;
	for(i = 0; byteCount-- > 0; i++) {
		if(i % 16 == 0) {
			if(i > 0)
				Video::printf("\n");
			Video::printf("%p:",ptr);
		}
		Video::printf(" %02x",*ptr);
		ptr++;
	}
}
