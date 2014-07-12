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

#include <common.h>
#include <task/uenv.h>
#include <task/proc.h>
#include <mem/useraccess.h>

ulong *UEnvBase::initProcStack(int argc,int envc,const char *args,size_t argsSize) {
	Thread *t = Thread::getRunning();

	/*
	 * Initial stack:
	 * +------------------+  <- top
	 * |       errno      |
	 * +------------------+
	 * |        TLS       | (pointer to the actual TLS)
	 * +------------------+
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
	/* finally we need errno, TLS, envc, envv, argc, argv and entryPoint */
	totalSize += sizeof(ulong) * 7;

	/* get sp */
	ulong *sp;
	t->getStackRange(NULL,(uintptr_t*)&sp,0);
	if(!PageDir::isInUserSpace((uintptr_t)sp - totalSize,totalSize))
		return NULL;

	/* space for errno and TLS */
	sp -= 3;

	/* copy arguments on the user-stack (4byte space) */
	char **argv = copyArgs(argc,args,sp);
	char **envv = copyArgs(envc,args,sp);

	/* align it by 16 byte (SSE) */
	sp = (ulong*)(ROUND_DN((uintptr_t)sp,16) - 8);

	/* store envc, envv, argc and argv */
	UserAccess::writeVar(sp--,(ulong)envv);
	UserAccess::writeVar(sp--,(ulong)envc);
	UserAccess::writeVar(sp--,(ulong)argv);
	UserAccess::writeVar(sp--,(ulong)argc);
	/* add entrypoint */
	UserAccess::writeVar(sp,0UL);
	return sp;
}

ulong *UEnvBase::initThreadStack(const void *arg,uintptr_t entry) {
	Thread *t = Thread::getRunning();

	/*
	 * Initial stack:
	 * +------------------+  <- top
	 * |       errno      |
	 * +------------------+
	 * |        TLS       | (pointer to the actual TLS)
	 * +------------------+
	 * |        arg       |
	 * +------------------+
	 * |    entryPoint    |  0 for initial thread, thread-entrypoint for others
	 * +------------------+
	 */

	size_t totalSize = 16 + 4 * sizeof(ulong);

	/* get sp */
	ulong *sp;
	t->getStackRange(NULL,(uintptr_t*)&sp,0);
	if(!PageDir::isInUserSpace((uintptr_t)sp - totalSize,totalSize)) {
		Proc::terminate(1,SIGSEGV);
		A_UNREACHED;
	}

	/* space for errno and TLS */
	sp -= 3;

	/* align it by 16 byte (SSE) */
	sp = (ulong*)ROUND_DN((uintptr_t)sp,16);

	/* put arg on stack */
	UserAccess::writeVar(sp--,(ulong)arg);
	/* add entrypoint */
	UserAccess::writeVar(sp,entry);
	return sp;
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
