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
#include <sys/mem/pagedir.h>
#include <sys/mem/kheap.h>
#include <sys/mem/virtmem.h>
#include <sys/dbg/kb.h>
#include <sys/dbg/console.h>
#include <sys/interrupts.h>
#include <sys/ksymbols.h>
#include <sys/video.h>
#include <sys/videolog.h>
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
	SpinLock::acquire(&randLock);
	lastRand = randa * lastRand + randc;
	int res = (int)((uint)(lastRand / 65536) % 32768);
	SpinLock::release(&randLock);
	return res;
}

void Util::srand(uint seed) {
	SpinLock::acquire(&randLock);
	lastRand = seed;
	SpinLock::release(&randLock);
}

void Util::startTimer() {
	profStart = CPU::rdtsc();
}

void Util::stopTimer(OStream &os,const char *prefix,...) {
	va_list l;
	uLongLong diff;
	diff.val64 = CPU::rdtsc() - profStart;
	va_start(l,prefix);
	os.vwritef(prefix,l);
	va_end(l);
	os.writef(": 0x%08x%08x\n",diff.val32.upper,diff.val32.lower);
}

void Util::panic(const char *fmt, ...) {
	va_list ap;
	va_start(ap,fmt);
	Util::vpanic(fmt,ap);
	va_end(ap);
}

void Util::vpanic(const char *fmt,va_list ap) {
	VideoLog vl;
	Video &vid = Video::get();
	Log &log = Log::get();
	const Thread *t = Thread::getRunning();
	panicArch();

	vid.clearScreen();

	/* print message */
	vl.writef("\n\033[co;7;4]PANIC: ");
	vl.vwritef(fmt,ap);
	vl.writef("%|s\033[co]\n","");

	/* write information about the running thread to log/screen */
	if(t != NULL)
		vl.writef("Caused by thread %d (%s)\n\n",t->getTid(),t->getProc()->getProgram());
	printStackTrace(vl,getKernelStackTrace());
	if(t) {
		printUserState(vl);

		log.writef("\n============= snip =============\n");
		log.writef("Region overview:\n");
		t->getProc()->getVM()->printShort(log,"\t");
		printStackTrace(vl,getUserStackTrace());
		log.writef("============= snip =============\n\n");
	}

	/* write into log only */
	if(t) {
		vid.writef("\n\nWriting regions and page-directory of the current process to log...");
		t->getProc()->getVM()->printRegions(log);
		t->getProc()->getPageDir()->print(log,PD_PART_USER);
		vid.writef("Done\n");
	}

	vid.writef("\nPress any key to start debugger");
	while(1) {
		Keyboard::get(NULL,KEV_PRESS,true);
		Console::start(NULL);
	}
}

void Util::printEventTrace(OStream &os,const FuncCall *trace,const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	os.vwritef(fmt,ap);
	va_end(ap);
	if(trace) {
		for(size_t i = 0; trace->addr != 0 && i < 5; i++) {
			KSymbols::Symbol *sym = KSymbols::getSymbolAt(trace->addr);
			if(sym)
				os.writef("%s",sym->funcName);
			else
				os.writef("%Px",trace->addr);
			trace++;
			if(trace->addr)
				os.writef(" ");
		}
	}
	os.writef("\n");
}

void Util::printStackTrace(OStream &os,const FuncCall *trace) {
	if(trace && trace->addr) {
		if(trace->addr < KERNEL_AREA)
			os.writef("User-Stacktrace:\n");
		else
			os.writef("Kernel-Stacktrace:\n");

		while(trace->addr != 0) {
			os.writef("\t%p -> %p (%s)\n",(trace + 1)->addr,trace->funcAddr,trace->funcName);
			trace++;
		}
	}
}

void Util::dumpMem(OStream &os,const void *addr,size_t dwordCount) {
	ulong *ptr = (ulong*)addr;
	while(dwordCount-- > 0) {
		os.writef("%p: 0x%0*lx\n",ptr,sizeof(ulong) * 2,*ptr);
		ptr++;
	}
}

void Util::dumpBytes(OStream &os,const void *addr,size_t byteCount) {
	uchar *ptr = (uchar*)addr;
	for(size_t i = 0; byteCount-- > 0; i++) {
		if(i % 16 == 0) {
			if(i > 0)
				os.writef("\n");
			os.writef("%p:",ptr);
		}
		os.writef(" %02x",*ptr);
		ptr++;
	}
}
