/**
 * $Id$
 */

#include <esc/common.h>
#include <sys/arch/eco32/boot.h>
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
