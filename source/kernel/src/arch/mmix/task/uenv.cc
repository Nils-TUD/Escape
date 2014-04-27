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
#include <sys/mem/useraccess.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/openfile.h>
#include <sys/boot.h>
#include <sys/log.h>
#include <sys/cpu.h>
#include <sys/video.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

void UEnv::startSignalHandler(Thread *t,int sig,Signals::handler_func handler) {
	IntrptStackFrame *curStack = t->getIntrptStack();
	KSpecRegs *sregs = t->getSpecRegs();

	ulong *sp = (ulong*)curStack[-15];				/* $254 */
	if(!PageDir::isInUserSpace((uintptr_t)(sp - 9),9 * sizeof(ulong)))
		goto error;

	/* backup rBB, rWW, rXX, rYY and rZZ */
	UserAccess::write(sp - 5,sregs,sizeof(KSpecRegs));
	sp -= 6;
	UserAccess::writeVar(sp--,(ulong)curStack[-9]);		/* rJ */
	UserAccess::writeVar(sp--,(ulong)handler);
	UserAccess::writeVar(sp,(ulong)sig);
	curStack[-15] = (ulong)sp;						/* $254 */

	if(t->isFaulted())
		goto error;

	/* jump to sigRetAddr for setup and finish-code */
	sregs->rww = t->getProc()->getSigRetAddr();
	sregs->rxx = 1UL << 63;
	return;

error:
	Proc::terminate(1,SIG_SEGFAULT);
	A_UNREACHED;
}

int UEnvBase::finishSignalHandler(A_UNUSED IntrptStackFrame *stack) {
	Thread *t = Thread::getRunning();
	IntrptStackFrame *curStack = t->getIntrptStack();
	ulong *sp = (ulong*)(curStack[-15]);			/* $254 */

	/* restore rBB, rWW, rXX, rYY and rZZ */
	sp += 2;
	UserAccess::readVar((ulong*)(curStack - 9),sp++);			/* rJ */
	KSpecRegs *sregs = t->getSpecRegs();
	UserAccess::read(sregs,sp,sizeof(KSpecRegs));
	sp += 5;
	curStack[-15] = (ulong)sp;						/* $254 */
	return 0;
}

bool UEnvBase::setupProc(int argc,int envc,const char *args,A_UNUSED size_t argsSize,
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
	 * $4 = envc
	 * $5 = envv
	 * $7 = entryPoint (0 for initial thread, thread-entrypoint for others)
	 * $8 = TLSStart (0 if not present)
	 * $9 = TLSSize (0 if not present)
	 */

	/* we need to know the total number of bytes we'll store on the stack */
	size_t totalSize = 0;
	if(argc > 0 || envc > 0) {
		/* first round the size of the arguments up. then we need argc+1 pointer */
		totalSize += ROUND_UP(argsSize,sizeof(ulong));
		totalSize += sizeof(void*) * (argc + 1 + envc + 1);
	}
	/* finally we need the stack-end */
	totalSize += sizeof(ulong);

	/* get register-stack */
	ulong *rsp,*ssp;
	t->getStackRange((uintptr_t*)&rsp,NULL,0);
	/* get software-stack */
	t->getStackRange(NULL,(uintptr_t*)&ssp,1);

	/* copy arguments on the user-stack (8byte space) */
	ssp--;
	char **argv = copyArgs(argc,args,ssp);
	char **envv = copyArgs(envc,args,ssp);
	UserAccess::writeVar(ssp,info->stackBegin);

	/* store argc and argv */
	UserAccess::writeVar(rsp + 1,(ulong)argc);
	UserAccess::writeVar(rsp + 2,(ulong)argv);
	UserAccess::writeVar(rsp + 4,(ulong)envc);
	UserAccess::writeVar(rsp + 5,(ulong)envv);

	/* add TLS args and entrypoint; use prog-entry here because its always the entry of the
	 * program, not the dynamic-linker */
	UEnv::addArgs(t,info,rsp,ssp,entryPoint,0,false);
	if(t->isFaulted())
		return false;
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
	 * $7 = entryPoint (0 for initial thread, thread-entrypoint for others)
	 * $8 = TLSStart (0 if not present)
	 * $9 = TLSSize (0 if not present)
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
	ulong *rsp,*ssp;
	t->getStackRange((uintptr_t*)&rsp,NULL,0);
	/* get software-stack */
	t->getStackRange(NULL,(uintptr_t*)&ssp,1);

	/* store location to UNSAVE from and the thread-argument */
	UserAccess::writeVar(--ssp,sinfo.stackBegin);
	UserAccess::writeVar(rsp + 1,(ulong)arg);

	/* add TLS args and entrypoint */
	UEnv::addArgs(t,&sinfo,rsp,ssp,t->getProc()->getEntryPoint(),tentryPoint,true);
	if(t->isFaulted())
		return NULL;
	return ssp;
}

void UEnv::addArgs(Thread *t,const ELF::StartupInfo *info,ulong *rsp,ulong *ssp,
                   uintptr_t entry,uintptr_t tentry,bool thread) {
	/* put address and size of the tls-region on the stack */
	ulong tlsStart,tlsEnd;
	if(t->getTLSRange(&tlsStart,&tlsEnd)) {
		UserAccess::writeVar(rsp + 8,tlsStart);
		UserAccess::writeVar(rsp + 9,tlsEnd - tlsStart);
	}
	else {
		/* no tls */
		UserAccess::writeVar(rsp + 8,(ulong)0);
		UserAccess::writeVar(rsp + 9,(ulong)0);
	}
	UserAccess::writeVar(rsp + 7,(ulong)(thread ? tentry : 0));

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
	UserAccess::writeVar(rsp + 10 + (255 - rg),(ulong)ssp);
	UserAccess::writeVar(rsp + 10 + (255 - rg) + 1,(ulong)ssp);
}
