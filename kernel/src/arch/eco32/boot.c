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
