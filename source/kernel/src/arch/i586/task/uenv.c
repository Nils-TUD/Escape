/**
 * $Id$
 */

#include <esc/common.h>
#include <sys/arch/i586/gdt.h>
#include <sys/task/uenv.h>
#include <sys/task/thread.h>
#include <sys/mem/vmm.h>
#include <sys/mem/paging.h>
#include <sys/vfs/real.h>
#include <string.h>
#include <assert.h>

static void uenv_setupStack(sIntrptStackFrame *frame,uintptr_t entryPoint);
static uint32_t *uenv_addArgs(const sThread *t,uint32_t *esp,uintptr_t tentryPoint,bool newThread);

bool uenv_setupProc(sIntrptStackFrame *frame,const char *path,
		int argc,const char *args,size_t argsSize,const sStartupInfo *info,uintptr_t entryPoint) {
	uint32_t *esp;
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
	 * |     TLSSize      |  0 if not present
	 * +------------------+
	 * |     TLSStart     |  0 if not present
	 * +------------------+
	 * |    entryPoint    |  0 for initial thread, thread-entrypoint for others
	 * +------------------+
	 */

	/* we need to know the total number of bytes we'll store on the stack */
	totalSize = 0;
	if(argc > 0) {
		/* first round the size of the arguments up. then we need argc+1 pointer */
		totalSize += (argsSize + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1);
		totalSize += sizeof(void*) * (argc + 1);
	}
	/* finally we need argc, argv, tlsSize, tlsStart and entryPoint */
	totalSize += sizeof(uint32_t) * 5;

	/* get esp */
	vmm_getRegRange(t->proc,t->stackRegion,NULL,(uintptr_t*)&esp);

	/* extend the stack if necessary */
	if(thread_extendStack((uintptr_t)esp - totalSize) < 0)
		return false;
	/* will handle copy-on-write */
	paging_isRangeUserWritable((uintptr_t)esp - totalSize,totalSize);

	/* copy arguments on the user-stack (4byte space) */
	esp--;
	argv = NULL;
	if(argc > 0) {
		char *str;
		int i;
		size_t len;
		argv = (char**)(esp - argc);
		/* space for the argument-pointer */
		esp -= argc;
		/* start for the arguments */
		str = (char*)esp;
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
		esp = (uint32_t*)(((uintptr_t)str & ~(sizeof(uint32_t) - 1)) - sizeof(uint32_t));
	}

	/* store argc and argv */
	*esp-- = (uintptr_t)argv;
	*esp-- = argc;
	/* add TLS args and entrypoint; use prog-entry here because its always the entry of the
	 * program, not the dynamic-linker */
	esp = uenv_addArgs(t,esp,info->progEntry,false);

	/* if its the dynamic linker, open the program to exec and give him the filedescriptor,
	 * so that he can load it including all shared libraries */
	if(info->linkerEntry != info->progEntry) {
		tFileNo file;
		tFD fd = proc_getFreeFd();
		if(fd < 0)
			return false;
		file = vfs_real_openPath(t->proc->pid,VFS_READ,path);
		if(file < 0)
			return false;
		assert(proc_assocFd(fd,file) == 0);
		*--esp = fd;
	}

	frame->uesp = (uint32_t)esp;
	frame->ebp = frame->uesp;
	uenv_setupStack(frame,entryPoint);
	return true;
}

bool uenv_setupThread(sIntrptStackFrame *frame,const void *arg,uintptr_t tentryPoint) {
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
	return true;
}

static void uenv_setupStack(sIntrptStackFrame *frame,uintptr_t entryPoint) {
	/* user-mode segments */
	frame->cs = SEGSEL_GDTI_UCODE | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->ds = SEGSEL_GDTI_UDATA | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->es = SEGSEL_GDTI_UDATA | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->fs = SEGSEL_GDTI_UDATA | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	/* gs is used for TLS */
	frame->gs = SEGSEL_GDTI_UTLS | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->uss = SEGSEL_GDTI_UDATA | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->eip = entryPoint;
	/* general purpose register */
	frame->eax = 0;
	frame->ebx = 0;
	frame->ecx = 0;
	frame->edx = 0;
	frame->esi = 0;
	frame->edi = 0;
}

static uint32_t *uenv_addArgs(const sThread *t,uint32_t *esp,uintptr_t tentryPoint,bool newThread) {
	/* put address and size of the tls-region on the stack */
	if(t->tlsRegion >= 0) {
		uintptr_t tlsStart,tlsEnd;
		vmm_getRegRange(t->proc,t->tlsRegion,&tlsStart,&tlsEnd);
		*esp-- = tlsEnd - tlsStart;
		*esp-- = tlsStart;
	}
	else {
		/* no tls */
		*esp-- = 0;
		*esp-- = 0;
	}

	*esp = newThread ? tentryPoint : 0;
	return esp;
}
