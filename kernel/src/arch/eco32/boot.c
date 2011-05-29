/**
 * $Id$
 */

#include <esc/common.h>
#include <sys/boot.h>
#include <sys/video.h>
#include <string.h>

static sLoadProg progs[MAX_PROG_COUNT];
static sBootInfo info;

void boot_init(const sBootInfo *binfo) {
	/* make a copy of the bootinfo, since the location it is currently stored in will be overwritten
	 * shortly */
	memcpy(&info,binfo,sizeof(sBootInfo));
	info.progs = progs;
	memcpy((void*)info.progs,binfo->progs,sizeof(sLoadProg) * binfo->progCount);
}

const sBootInfo *boot_getInfo(void) {
	return &info;
}

size_t boot_getKernelSize(void) {
	return progs[0].size;
}

size_t boot_getModuleSize(void) {
	uintptr_t start = progs[1].start;
	uintptr_t end = progs[info.progCount - 1].start + progs[info.progCount - 1].size;
	return end - start;
}

size_t boot_getUsableMemCount(void) {
	return info.memSize;
}

void boot_loadModules(sIntrptStackFrame *stack) {
	/* TODO */
#if 0
	size_t i;
	tPid pid;
	tInodeNo nodeNo;
	sModule *mod = mb->modsAddr;

	/* it's not good to do this twice.. */
	if(loadedMods)
		return;

	/* start idle-thread */
	if(proc_startThread(0,NULL) == thread_getRunning()->tid) {
		thread_idle();
		util_panic("Idle returned");
	}

	loadedMods = true;
	for(i = 0; i < mb->modsCount; i++) {
		/* parse args */
		int argc;
		const char **argv = boot_parseArgs(mod->name,&argc);
		if(argc < 2)
			util_panic("Invalid arguments for multiboot-module: %s\n",mod->name);

		/* clone proc */
		pid = proc_getFreePid();
		if(pid == INVALID_PID)
			util_panic("No free process-slots");

		if(proc_clone(pid,0)) {
			sStartupInfo info;
			size_t argSize = 0;
			char *argBuffer = NULL;
			sProc *p = proc_getRunning();
			/* remove regions (except stack) */
			proc_removeRegions(p,false);
			/* now load module */
			memcpy(p->command,argv[0],strlen(argv[0]) + 1);
			if(elf_loadFromMem((void*)mod->modStart,mod->modEnd - mod->modStart,&info) < 0)
				util_panic("Loading multiboot-module %s failed",p->command);
			/* build args */
			argc = proc_buildArgs(argv,&argBuffer,&argSize,false);
			if(argc < 0)
				util_panic("Building args for multiboot-module %s failed: %d",p->command,argc);
			/* no dynamic linking here */
			p->entryPoint = info.progEntry;
			if(!uenv_setupProc(stack,p->command,argc,argBuffer,argSize,&info,info.progEntry))
				util_panic("Unable to setup user-stack for multiboot module %s",p->command);
			kheap_free(argBuffer);
			/* we don't want to continue the loop ;) */
			return;
		}

		/* wait until the driver is registered */
		vid_printf("Loading '%s'...\n",argv[0]);
		/* don't create a pipe- or driver-usage-node here */
		while(vfs_node_resolvePath(argv[1],&nodeNo,NULL,VFS_NOACCESS) < 0) {
			/* Note that we HAVE TO sleep here because we may be waiting for ata and fs is not
			 * started yet. I.e. if ata calls sleep() there is no other runnable thread (except
			 * idle, but its just chosen if nobody else wants to run), so that we wouldn't make
			 * a switch but stay here for ever (and get no timer-interrupts to wakeup ata) */
			timer_sleepFor(thread_getRunning()->tid,20);
			thread_switch();
		}

		mod++;
	}

	/* start the swapper-thread. it will never return */
	if(proc_startThread(0,NULL) == thread_getRunning()->tid) {
		swap_start();
		util_panic("Swapper reached this");
	}

	/* create the vm86-task */
	assert(vm86_create() == 0);
#endif
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void boot_dbg_print(void) {
	size_t i;
	vid_printf("Memory size: %u bytes\n",info.memSize);
	vid_printf("Disk size: %u bytes\n",info.diskSize);
	vid_printf("Boot modules:\n");
	/* skip kernel */
	for(i = 1; i < info.progCount; i++) {
		vid_printf("\t%s [%08x .. %08x]\n",info.progs[i].path,
				info.progs[i].start,info.progs[i].start + info.progs[i].size);
	}
}

#endif
