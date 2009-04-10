/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <util.h>
#include <intrpt.h>
#include <proc.h>
#include <ksymbols.h>
#include <paging.h>
#include <video.h>
#include <stdarg.h>
#include <string.h>

/* the x86-call instruction is 5 bytes long */
#define CALL_INSTR_SIZE 5

/* helper-functions */
static u32 util_sprintu(char *str,u32 n,u8 base);
static u32 util_sprintn(char *str,s32 n);
static u32 util_sprintfPad(char *str,char c,s32 count);

/* the beginning of the kernel-stack */
extern u32 kernelStack;

/* for ksprintf */
static char hexCharsBig[] = "0123456789ABCDEF";

void util_panic(const char *fmt,...) {
	sProc *p = proc_getRunning();
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
	vid_restoreColor();
	vid_printf("Caused by process %d (%s)\n\n",p->pid,p->command);
	util_printStackTrace(util_getKernelStackTrace());
	util_printStackTrace(util_getUserStackTrace(p,intrpt_getCurStack()));
	intrpt_setEnabled(false);
	util_halt();
}

sFuncCall *util_getUserStackTrace(sProc *p,sIntrptStackFrame *stack) {
	return util_getStackTrace((u32*)stack->ebp,
			KERNEL_AREA_V_ADDR - p->stackPages * PAGE_SIZE,KERNEL_AREA_V_ADDR);
}

sFuncCall *util_getKernelStackTrace(void) {
	u32 start,end;
	u32* ebp = (u32*)getStackFrameStart();

	/* determine the stack-bounds; we have a temp stack at the beginning */
	if((u32)ebp >= KERNEL_STACK && (u32)ebp < KERNEL_STACK + PAGE_SIZE) {
		start = KERNEL_STACK;
		end = KERNEL_STACK + PAGE_SIZE;
	}
	else {
		start = ((u32)&kernelStack) - TMP_STACK_SIZE;
		end = (u32)&kernelStack;
	}

	return util_getStackTrace(ebp,start,end);
}

sFuncCall *util_getStackTrace(u32 *ebp,u32 start,u32 end) {
	static sFuncCall frames[MAX_STACK_DEPTH];
	u32 i;
	bool isKernel = (u32)ebp >= KERNEL_AREA_V_ADDR;
	sFuncCall *frame = &frames[0];
	sSymbol *sym;

	for(i = 0; i < MAX_STACK_DEPTH; i++) {
		/* prevent page-fault */
		if((u32)ebp < start || (u32)ebp >= end)
			break;
		frame->addr = *(ebp + 1) - CALL_INSTR_SIZE;
		if(isKernel) {
			sym = ksym_getSymbolAt(frame->addr);
			frame->funcAddr = sym->address;
			frame->funcName = sym->funcName;
		}
		else {
			frame->funcAddr = frame->addr;
			frame->funcName = "Unknown";
		}
		ebp = (u32*)*ebp;
		frame++;
	}

	/* terminate */
	frame->addr = 0;
	return &frames[0];
}

void util_printStackTrace(sFuncCall *trace) {
	if(trace->addr < KERNEL_AREA_V_ADDR)
		vid_printf("User-Stacktrace:\n");
	else
		vid_printf("Kernel-Stacktrace:\n");

	/* TODO maybe we should skip util_printStackTrace here? */
	while(trace->addr != 0) {
		vid_printf("\t0x%08x -> 0x%08x (%s)\n",(trace + 1)->addr,trace->funcAddr,trace->funcName);
		trace++;
	}
}

void util_dumpMem(void *addr,u32 dwordCount) {
	u32 *ptr = (u32*)addr;
	while(dwordCount-- > 0) {
		vid_printf("0x%x: 0x%08x\n",ptr,*ptr);
		ptr++;
	}
}

void util_dumpBytes(void *addr,u32 byteCount) {
	u32 i = 0;
	u8 *ptr = (u8*)addr;
	for(i = 0; byteCount-- > 0; i++) {
		vid_printf("%02x ",*ptr);
		ptr++;
		if(i % 12 == 11)
			vid_printf("\n");
	}
}

void util_sprintf(char *str,const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	util_vsprintf(str,fmt,ap);
	va_end(ap);
}

void util_vsprintf(char *str,const char *fmt,va_list ap) {
	char c,b,padchar;
	char *s;
	u8 pad;
	s32 n;
	u32 u,x;
	bool readFlags,padRight;
	u8 base;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* finished? */
			if(c == '\0') {
				*str = '\0';
				return;
			}
			*str++ = c;
		}

		/* read flags */
		padchar = ' ';
		padRight = false;
		readFlags = true;
		while(readFlags) {
			switch(*fmt) {
				case '-':
					padRight = true;
					fmt++;
					break;
				case '0':
					padchar = '0';
					fmt++;
					break;
				default:
					readFlags = false;
					break;
			}
		}

		/* read pad-width */
		pad = 0;
		while(*fmt >= '0' && *fmt <= '9') {
			pad = pad * 10 + (*fmt - '0');
			fmt++;
		}

		/* determine format */
		switch(c = *fmt++) {
			/* signed integer */
			case 'd':
				n = va_arg(ap, s32);
				if(n < 0) {
					*str++ = '-';
					n = -n;
				}
				if(!padRight && pad > 0)
					str += util_sprintfPad(str,padchar,pad - util_getnwidth(n));
				x = util_sprintn(str,n);
				str += x;
				if(padRight && pad > 0)
					str += util_sprintfPad(str,padchar,pad - x);
				break;
			/* unsigned integer */
			case 'b':
			case 'u':
			case 'o':
			case 'x':
				u = va_arg(ap, u32);
				base = c == 'o' ? 8 : (c == 'x' ? 16 : (c == 'b' ? 2 : 10));
				if(!padRight && pad > 0)
					str += util_sprintfPad(str,padchar,pad - util_getuwidth(u,base));
				x = util_sprintu(str,u,base);
				str += x;
				if(padRight && pad > 0)
					str += util_sprintfPad(str,padchar,pad - x);
				break;
			/* string */
			case 's':
				s = va_arg(ap, char*);
				if(!padRight && pad > 0)
					str += util_sprintfPad(str,padchar,pad - strlen(s));
				x = 0;
				while(*s) {
					*str++ = *s++;
					x++;
				}
				if(padRight && pad > 0)
					str += util_sprintfPad(str,padchar,pad - x);
				break;
			/* character */
			case 'c':
				b = (char)va_arg(ap, u32);
				*str++ = b;
				break;
			/* all other */
			default:
				*str++ = c;
				break;
		}
	}
}

u8 util_getuwidth(u32 n,u8 base) {
	u8 width = 1;
	while(n >= base) {
		n /= base;
		width++;
	}
	return width;
}

u8 util_getnwidth(s32 n) {
	/* we have at least one char */
	u8 width = 1;
	if(n < 0) {
		width++;
		n = -n;
	}
	while(n >= 10) {
		n /= 10;
		width++;
	}
	return width;
}

static u32 util_sprintu(char *str,u32 n,u8 base) {
	u32 c = 0;
	if(n >= base) {
		c += util_sprintu(str,n / base,base);
		str += c;
	}
	*str = hexCharsBig[n % base];
	return c + 1;
}

static u32 util_sprintn(char *str,s32 n) {
	u32 c = 0;
	if(n >= 10) {
		c += util_sprintn(str,n / 10);
		str += c;
	}
	*str = '0' + n % 10;
	return c + 1;
}

static u32 util_sprintfPad(char *str,char c,s32 count) {
	char *start = str;
	while(count-- > 0) {
		*str++ = c;
	}
	return str - start;
}
