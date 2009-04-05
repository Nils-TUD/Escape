/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <debug.h>
#include <cpu.h>
#include <video.h>

static u64 start = 0;

void dbg_startTimer(void) {
	start = cpu_rdtsc();
}

void dbg_stopTimer(const char *prefix) {
	u64 diff = cpu_rdtsc() - start;
	u32 *ptr = (u32*)&diff;
	vid_printf("%s: 0x%08x%08x\n",prefix,*(ptr + 1),*ptr);
}
