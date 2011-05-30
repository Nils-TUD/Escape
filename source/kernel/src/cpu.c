/**
 * $Id$
 */

#include <esc/common.h>
#include <sys/mem/kheap.h>
#include <sys/cpu.h>
#include <sys/video.h>

#if DEBUGGING

void cpu_dbg_print(void) {
	sStringBuffer buf;
	buf.dynamic = true;
	buf.len = 0;
	buf.size = 0;
	buf.str = NULL;
	cpu_sprintf(&buf);
	vid_printf("%s",buf.str);
	kheap_free(buf.str);
}

#endif
