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
#include <sys/task/proc.h>
#include <sys/mem/useraccess.h>

ulong *UEnvBase::initProcStack(int argc,int envc,const char *args,size_t argsSize,uintptr_t entry) {
	Thread *t = Thread::getRunning();

	/*
	 * Initial stack:
	 * +------------------+  <- top
	 * |     arguments    |
	 * |        ...       |
	 * +------------------+
	 * |       envv       |
	 * +------------------+
	 * |       envc       |
	 * +------------------+
	 * |       argv       |
	 * +------------------+
	 * |       argc       |
	 * +------------------+
	 * |     TLSSize      |  0 if not present
	 * +------------------+
	 * |     TLSStart     |  0 if not present
	 * +------------------+
	 * |    entryPoint    |  0 for initial thread, thread-entrypoint for others
	 * +------------------+
	 */

	/* we need to know the total number of bytes we'll store on the stack */
	size_t totalSize = 0;
	if(argc > 0 || envc > 0) {
		/* first round the size of the arguments up. then we need argc+1 pointer */
		totalSize += ROUND_UP(argsSize,sizeof(ulong));
		totalSize += sizeof(void*) * (argc + 1 + envc + 1);
	}
	totalSize = ROUND_UP(totalSize,16) + 8;
	/* finally we need envc, envv, argc, argv, tlsSize, tlsStart and entryPoint */
	totalSize += sizeof(ulong) * 7;

	/* get sp */
	ulong *sp;
	t->getStackRange(NULL,(uintptr_t*)&sp,0);
	if(!PageDir::isInUserSpace((uintptr_t)sp - totalSize,totalSize))
		return NULL;

	/* copy arguments on the user-stack (4byte space) */
	sp--;
	char **argv = copyArgs(argc,args,sp);
	char **envv = copyArgs(envc,args,sp);

	/* align it by 16 byte (SSE) */
	sp = (ulong*)(ROUND_DN((uintptr_t)sp,16) - 8);

	/* store envc, envv, argc and argv */
	UserAccess::writeVar(sp--,(ulong)envv);
	UserAccess::writeVar(sp--,(ulong)envc);
	UserAccess::writeVar(sp--,(ulong)argv);
	UserAccess::writeVar(sp--,(ulong)argc);
	/* add TLS args and entrypoint; use prog-entry here because its always the entry of the
	 * program, not the dynamic-linker */
	return addArgs(t,sp,entry,false);
}

ulong *UEnvBase::initThreadStack(const void *arg,uintptr_t entry) {
	Thread *t = Thread::getRunning();

	/*
	 * Initial stack:
	 * +------------------+  <- top
	 * |       arg        |
	 * +------------------+
	 * |     TLSSize      |  0 if not present
	 * +------------------+
	 * |     TLSStart     |  0 if not present
	 * +------------------+
	 * |    entryPoint    |  0 for initial thread, thread-entrypoint for others
	 * +------------------+
	 */

	size_t totalSize = 16 + 4 * sizeof(ulong);

	/* get sp */
	ulong *sp;
	t->getStackRange(NULL,(uintptr_t*)&sp,0);
	if(!PageDir::isInUserSpace((uintptr_t)sp - totalSize,totalSize)) {
		Proc::terminate(1,SIG_SEGFAULT);
		A_UNREACHED;
	}

	/* align it by 16 byte (SSE) */
	sp = (ulong*)((uintptr_t)sp - 16);

	/* put arg on stack */
	UserAccess::writeVar(sp--,(ulong)arg);
	/* add TLS args and entrypoint */
	return addArgs(t,sp,entry,true);
}

char **UEnvBase::copyArgs(int argc,const char *&args,ulong *&sp) {
	char **argv = NULL;
	if(argc > 0) {
		/* space for the argument-pointer */
		sp -= argc;
		argv = (char**)sp;
		/* start for the arguments */
		char *str = (char*)sp;
		for(int i = 0; i < argc; i++) {
			/* start <len> bytes backwards */
			size_t len = strlen(args) + 1;
			str -= len;
			/* store arg-pointer and copy arg */
			UserAccess::writeVar(argv + i,str);
			UserAccess::write(str,args,len);
			/* to next */
			args += len;
		}
		/* ensure that we don't overwrites the characters */
		sp = (ulong*)(((uintptr_t)str & ~(sizeof(ulong) - 1)) - sizeof(ulong));
	}
	return argv;
}

ulong *UEnvBase::addArgs(Thread *t,ulong *sp,uintptr_t tentryPoint,bool newThread) {
	/* put address and size of the tls-region on the stack */
	uintptr_t tlsStart,tlsEnd;
	if(t->getTLSRange(&tlsStart,&tlsEnd)) {
		UserAccess::writeVar(sp--,(ulong)(tlsEnd - tlsStart));
		UserAccess::writeVar(sp--,(ulong)tlsStart);
	}
	else {
		/* no tls */
		UserAccess::writeVar(sp--,(ulong)0);
		UserAccess::writeVar(sp--,(ulong)0);
	}

	UserAccess::writeVar(sp,(ulong)(newThread ? tentryPoint : 0));
	return sp;
}
