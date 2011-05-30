/**
 * $Id$
 */

#include <esc/common.h>
#include <sys/task/uenv.h>
#include <sys/task/thread.h>
#include <sys/mem/vmm.h>
#include <sys/mem/paging.h>
#include <string.h>
#include <assert.h>

static void uenv_addArgs(const sThread *t,sIntrptStackFrame *frame,uintptr_t tentryPoint,
		bool newThread);

bool uenv_setupProc(sIntrptStackFrame *frame,const char *path,
		int argc,const char *args,size_t argsSize,const sStartupInfo *info,uintptr_t entryPoint) {
	uint32_t *sp;
	char **argv;
	size_t totalSize;
	sThread *t = thread_getRunning();
	vassert(frame != NULL,"frame == NULL");

	/*
	 * Initial stack:
	 * +------------------+  <- top
	 * |     arguments    |
	 * |        ...       |
	 * +------------------+
	 * |       argv       |
	 * +------------------+
	 * |       argc       |
	 * +------------------+
	 *
	 * Registers:
	 * $4 = entryPoint (0 for initial thread, thread-entrypoint for others)
	 * $5 = TLSStart (0 if not present)
	 * $6 = TLSSize (0 if not present)
	 */

	/* we need to know the total number of bytes we'll store on the stack */
	totalSize = 0;
	if(argc > 0) {
		/* first round the size of the arguments up. then we need argc+1 pointer */
		totalSize += (argsSize + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1);
		totalSize += sizeof(void*) * (argc + 1);
	}
	/* finally we need argc and argv */
	totalSize += sizeof(uint32_t) * 2;

	/* get esp */
	vmm_getRegRange(t->proc,t->stackRegion,NULL,(uintptr_t*)&sp);

	/* extend the stack if necessary */
	if(thread_extendStack((uintptr_t)sp - totalSize) < 0)
		return false;
	/* will handle copy-on-write */
	paging_isRangeUserWritable((uintptr_t)sp - totalSize,totalSize);

	/* copy arguments on the user-stack (4byte space) */
	sp--;
	argv = NULL;
	if(argc > 0) {
		char *str;
		int i;
		size_t len;
		argv = (char**)(sp - argc);
		/* space for the argument-pointer */
		sp -= argc;
		/* start for the arguments */
		str = (char*)sp;
		for(i = 0; i < argc; i++) {
			/* start <len> bytes backwards */
			len = strlen(args) + 1;
			str -= len;
			/* store arg-pointer and copy arg */
			argv[i] = str;
			memcpy(str,args,len);
			/* to next */
			args += len;
		}
		/* ensure that we don't overwrites the characters */
		sp = (uint32_t*)(((uintptr_t)str & ~(sizeof(uint32_t) - 1)) - sizeof(uint32_t));
	}

	/* store argc and argv */
	*sp-- = (uintptr_t)argv;
	*sp = argc;
	/* add TLS args and entrypoint; use prog-entry here because its always the entry of the
	 * program, not the dynamic-linker */
	uenv_addArgs(t,frame,info->progEntry,false);

	/* set entry-point and stack-pointer */
	frame->r[29] = (uint32_t)sp;
	frame->r[30] = entryPoint - 4; /* we'll skip the trap-instruction for syscalls */
	return true;
}

bool uenv_setupThread(sIntrptStackFrame *frame,const void *arg,uintptr_t tentryPoint) {
#if 0
	uint32_t *esp;
	size_t totalSize = 3 * sizeof(uint32_t) + sizeof(void*);
	sThread *t = thread_getRunning();

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

	/* get esp */
	vmm_getRegRange(t->proc,t->stackRegion,NULL,(uintptr_t*)&esp);

	/* extend the stack if necessary */
	if(thread_extendStack((uintptr_t)esp - totalSize) < 0)
		return false;
	/* will handle copy-on-write */
	paging_isRangeUserWritable((uintptr_t)esp - totalSize,totalSize);

	/* put arg on stack */
	esp--;
	*esp-- = (uintptr_t)arg;
	/* add TLS args and entrypoint */
	esp = uenv_addArgs(t,esp,tentryPoint,true);

	frame->uesp = (uint32_t)esp;
	frame->ebp = frame->uesp;
	uenv_setupStack(frame,t->proc->entryPoint);
#endif
	return true;
}

static void uenv_addArgs(const sThread *t,sIntrptStackFrame *frame,uintptr_t tentryPoint,
		bool newThread) {
	/* put address and size of the tls-region on the stack */
	if(t->tlsRegion >= 0) {
		uintptr_t tlsStart,tlsEnd;
		vmm_getRegRange(t->proc,t->tlsRegion,&tlsStart,&tlsEnd);
		frame->r[5] = tlsStart;
		frame->r[6] = tlsEnd - tlsStart;
	}
	else {
		/* no tls */
		frame->r[5] = 0;
		frame->r[6] = 0;
	}

	frame->r[4] = newThread ? tentryPoint : 0;
}
