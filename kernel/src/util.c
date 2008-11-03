/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/util.h"
#include "../h/video.h"
#include "../h/intrpt.h"
#include <stdarg.h>

void panic(string fmt,...) {
	va_list ap;
	vid_printf("\n");
	vid_setLineBG(vid_getLine(),RED);
	vid_useColor(RED,WHITE);
	vid_printf("PANIC: ");

	/* print message */
	va_start(ap,fmt);
	vid_vprintf(fmt,ap);
	va_end(ap);

	vid_printf("\n");
	intrpt_disable();
	halt();
}

void dumpMem(void *addr,u32 dwordCount) {
	u32 *ptr = (u32*)addr;
	while(dwordCount-- > 0) {
		vid_printf("0x%x: 0x%x\n",ptr,*ptr);
		ptr++;
	}
}

void *memcpy(void *dest,const void *src,size_t len) {
	u8 *d = dest;
	const u8 *s = src;
	while(len--)
		*d++ = *s++;
	return dest;
}

s32 memcmp(const void *str1,const void *str2,size_t count) {
	const u8 *s1 = str1;
	const u8 *s2 = str2;

	while(count-- > 0) {
		if(*s1++ != *s2++)
			return s1[-1] < s2[-1] ? -1 : 1;
	}
	return 0;
}

void memset(void *addr,u32 value,size_t count) {
	u32 *ptr = (u32*)addr;
	while(count-- > 0) {
		*ptr++ = value;
	}
}
