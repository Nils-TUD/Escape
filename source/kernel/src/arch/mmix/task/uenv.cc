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

#include <sys/common.h>
#include <sys/task/uenv.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/pagedir.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/openfile.h>
#include <sys/boot.h>
#include <sys/log.h>
#include <sys/cpu.h>
#include <sys/video.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define KEYBOARD_BASE		0x8006000000000000
#define KEYBOARD_CTRL		0
#define KEYBOARD_IEN		0x02

int UEnvBase::finishSignalHandler(A_UNUSED IntrptStackFrame *stack) {
	Thread *t = Thread::getRunning();
	IntrptStackFrame *curStack = t->getIntrptStack();
	uint64_t *sp = (uint64_t*)(curStack[-15]);	/* $254 */

	/* restore rBB, rWW, rXX, rYY and rZZ */
	sp += 2;
	curStack[-9] = *sp++;			/* rJ */
	KSpecRegs *sregs = t->getSpecRegs();
	memcpy(sregs,sp,sizeof(KSpecRegs));
	sp += 5;
	curStack[-15] = (uint64_t)sp;	/* $254 */
	return 0;
}

bool UEnvBase::setupProc(int argc,const char *args,A_UNUSED size_t argsSize,
                         const ELF::StartupInfo *info,uintptr_t entryPoint,A_UNUSED int fd) {
	Thread *t = Thread::getRunning();

	/*
	 * Initial software stack:
	 * +------------------+  <- top
	 * |     arguments    |
	 * |        ...       |
	 * +------------------+
	 * |     stack-end    |  used for UNSAVE
	 * +------------------+
	 *
	 * Registers:
	 * $1 = argc
	 * $2 = argv
	 * $4 = entryPoint (0 for initial thread, thread-entrypoint for others)
	 * $5 = TLSStart (0 if not present)
	 * $6 = TLSSize (0 if not present)
	 */

	/* get register-stack */
	uint64_t *rsp,*ssp;
	t->getStackRange((uintptr_t*)&rsp,NULL,0);
	/* get software-stack */
	t->getStackRange(NULL,(uintptr_t*)&ssp,1);

	/* copy arguments on the user-stack (8byte space) */
	ssp--;
	char **argv = NULL;
	if(argc > 0) {
		argv = (char**)(ssp - argc);
		/* space for the argument-pointer */
		ssp -= argc;
		/* start for the arguments */
		char *str = (char*)ssp;
		for(int i = 0; i < argc; i++) {
			/* start <len> bytes backwards */
			size_t len = strlen(args) + 1;
			str -= len;
			/* store arg-pointer and copy arg */
			argv[i] = str;
			memcpy(str,args,len);
			/* to next */
			args += len;
		}
		/* ensure that we don't overwrites the characters */
		ssp = (uint64_t*)(((uintptr_t)str & ~(sizeof(uint64_t) - 1)) - sizeof(uint64_t));
	}
	*ssp = info->stackBegin;

	/* store argc and argv */
	rsp[1] = argc;
	rsp[2] = (uint64_t)argv;

	/* add TLS args and entrypoint; use prog-entry here because its always the entry of the
	 * program, not the dynamic-linker */
	UEnv::addArgs(t,info,rsp,ssp,entryPoint,0,false);
	return true;
}

void *UEnvBase::setupThread(const void *arg,uintptr_t tentryPoint) {
	Thread *t = Thread::getRunning();
	ELF::StartupInfo sinfo;
	sinfo.progEntry = t->getProc()->getEntryPoint();
	sinfo.linkerEntry = 0;
	sinfo.stackBegin = 0;

	/*
	 * Initial software stack:
	 * +------------------+  <- top
	 * |     stack-end    |  used for UNSAVE
	 * +------------------+
	 *
	 * Registers:
	 * $1 = arg
	 * $4 = entryPoint (0 for initial thread, thread-entrypoint for others)
	 * $5 = TLSStart (0 if not present)
	 * $6 = TLSSize (0 if not present)
	 */

	/* the thread has to perform an UNSAVE at the beginning to establish the initial state.
	 * therefore, we have to prepare this again with the ELF-finisher. additionally, we have to
	 * take care that we use ELF::finishFromMem() for boot-modules and ELF::finishFromFile() other-
	 * wise. (e.g. fs depends on rtc -> rtc can't read it from file because fs is not ready) */
	pid_t pid = t->getProc()->getPid();
	if(t->getProc()->getFlags() & P_BOOT) {
		const BootInfo *info = Boot::getInfo();
		for(size_t i = 1; i < info->progCount; i++) {
			if(info->progs[i].id == pid) {
				if(ELF::finishFromMem((void*)info->progs[i].start,info->progs[i].size,&sinfo) < 0)
					return NULL;
				break;
			}
		}
	}
	else {
		/* TODO well, its not really nice that we have to read this stuff again for every started
		 * thread :/ */
		/* every process has a text-region from his binary */
		VMRegion *textreg = t->getProc()->getVM()->getRegion(t->getProc()->getEntryPoint());
		assert(textreg->reg->getFile() != NULL);
		ssize_t res;
		sElfEHeader ehd;

		/* seek to header */
		if(textreg->reg->getFile()->seek(pid,0,SEEK_SET) < 0) {
			Log::get().writef("[LOADER] Unable to seek to header of '%s'\n",t->getProc()->getProgram());
			return NULL;
		}

		/* read the header */
		if((res = textreg->reg->getFile()->read(pid,&ehd,sizeof(sElfEHeader))) !=
				sizeof(sElfEHeader)) {
			Log::get().writef("[LOADER] Reading ELF-header of '%s' failed: %s\n",
					t->getProc()->getProgram(),strerror(res));
			return NULL;
		}

		res = ELF::finishFromFile(textreg->reg->getFile(),&ehd,&sinfo);
		if(res < 0)
			return NULL;
	}

	/* get register-stack */
	uint64_t *rsp,*ssp;
	t->getStackRange((uintptr_t*)&rsp,NULL,0);
	/* get software-stack */
	t->getStackRange(NULL,(uintptr_t*)&ssp,1);

	/* store location to UNSAVE from and the thread-argument */
	*--ssp = sinfo.stackBegin;
	rsp[1] = (uint64_t)arg;

	/* add TLS args and entrypoint */
	UEnv::addArgs(t,&sinfo,rsp,ssp,t->getProc()->getEntryPoint(),tentryPoint,true);
	return ssp;
}

void UEnv::startSignalHandler(Thread *t,int sig,Signals::handler_func handler) {
	IntrptStackFrame *curStack = t->getIntrptStack();
	uint64_t *sp = (uint64_t*)curStack[-15];	/* $254 */
	if(!PageDir::isInUserSpace((uintptr_t)(sp - 9),9 * sizeof(uint64_t))) {
		Proc::segFault();
		/* not reached */
		assert(false);
	}

	/* backup rBB, rWW, rXX, rYY and rZZ */
	KSpecRegs *sregs = t->getSpecRegs();
	memcpy(sp - 5,sregs,sizeof(KSpecRegs));
	sp -= 6;
	*sp-- = curStack[-9];			/* rJ */
	*sp-- = (uintptr_t)handler;
	*sp = sig;
	curStack[-15] = (uint64_t)sp;	/* $254 */

	/* jump to sigRetAddr for setup and finish-code */
	sregs->rww = t->getProc()->getSigRetAddr();
	sregs->rxx = 1UL << 63;
}

void UEnv::addArgs(Thread *t,const ELF::StartupInfo *info,uint64_t *rsp,uint64_t *ssp,
                   uintptr_t entry,uintptr_t tentry,bool thread) {
	/* put address and size of the tls-region on the stack */
	uintptr_t tlsStart,tlsEnd;
	if(t->getTLSRange(&tlsStart,&tlsEnd)) {
		rsp[5] = tlsStart;
		rsp[6] = tlsEnd - tlsStart;
	}
	else {
		/* no tls */
		rsp[5] = 0;
		rsp[6] = 0;
	}
	rsp[4] = thread ? tentry : 0;

	if(!thread) {
		/* setup sp and fp in kernel-stack; we pass the location to unsave from with it */
		uint64_t *frame = t->getIntrptStack();
		frame[-(12 + 2 + 1)] = (uint64_t)ssp;
		frame[-(12 + 2 + 2)] = (uint64_t)ssp;

		/* setup start */
		KSpecRegs *sregs = t->getSpecRegs();
		sregs->rww = entry;
		sregs->rxx = 1UL << 63;
	}
	/* set them in the stack to unsave from in user-space */
	int rg = *(uint64_t*)info->stackBegin >> 56;
	rsp[6 + (255 - rg)] = (uint64_t)ssp;
	rsp[6 + (255 - rg) + 1] = (uint64_t)ssp;
}
