#include "../h/util.h"
#include "../h/video.h"
#include "../h/intrpt.h"

void panic(char *msg) {
	vid_printf("\n");
	vid_setLineBG(vid_getLine(),RED);
	vid_useColor(RED,WHITE);
	vid_printf("PANIC: %s\n",msg);
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

void memset(void *addr,u32 value,u32 count) {
	u32 *ptr = (u32*)addr;
	while(count-- > 0) {
		*ptr++ = value;
	}
}
