/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <common.h>
#include <machine/intrpt.h>
#include <task/proc.h>
#include <mem/pmem.h>
#include <mem/paging.h>
#include <mem/kheap.h>
#include <ksymbols.h>
#include <video.h>
#include <stdarg.h>
#include <string.h>
#include <register.h>
#include <util.h>

/* the x86-call instruction is 5 bytes long */
#define CALL_INSTR_SIZE			5

/* the minimum space-increase-size in util_vsprintf() */
#define SPRINTF_MIN_INC_SIZE	10

/* ensures that <buf> in util_vsprintf() has enough space for MAX(<width>,<pad>).
 * If necessary more space is allocated. If it fails false will be returned */
#define SPRINTF_INCREASE(width,pad) \
	if(buf->dynamic) { \
		u32 tmpLen = MAX((width),(pad)); \
		if(buf->len + tmpLen >= buf->size) { \
			buf->size += MAX(SPRINTF_MIN_INC_SIZE,1 + buf->len + tmpLen - buf->size); \
			buf->str = (char*)kheap_realloc(buf->str,buf->size * sizeof(char)); \
			if(!buf->str) { \
				buf->size = 0; \
				return false; \
			} \
		} \
	}

/* adds one char to <str> in util_vsprintf() and ensures that there is enough space for the char */
#define SPRINTF_ADD_CHAR(c) \
	SPRINTF_INCREASE(1,1); \
	*str++ = (c);

/* helper-functions */
static u32 util_sprintu(char *str,u32 n,u8 base);
static u32 util_sprintn(char *str,s32 n);
static u32 util_sprintfPad(char *str,char c,s32 count);

/* the beginning of the kernel-stack */
extern u32 kernelStack;

/* for ksprintf */
static char hexCharsBig[] = "0123456789ABCDEF";

void util_panic(const char *fmt,...) {
	u32 regs[REG_COUNT];
	sIntrptStackFrame *istack = intrpt_getCurStack();
	sThread *t = thread_getRunning();
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
	vid_printf("Caused by thread %d (%s)\n\n",t->tid,t->proc->command);
	util_printStackTrace(util_getKernelStackTrace());

	util_printStackTrace(util_getUserStackTrace(t,intrpt_getCurStack()));
	vid_printf("User-Register:\n");
	regs[R_EAX] = istack->eax;
	regs[R_EBX] = istack->ebx;
	regs[R_ECX] = istack->ecx;
	regs[R_EDX] = istack->edx;
	regs[R_ESI] = istack->esi;
	regs[R_EDI] = istack->edi;
	regs[R_ESP] = istack->uesp;
	regs[R_EBP] = istack->ebp;
	regs[R_CS] = istack->cs;
	regs[R_DS] = istack->ds;
	regs[R_ES] = istack->es;
	regs[R_FS] = istack->fs;
	regs[R_GS] = istack->gs;
	regs[R_SS] = istack->uss;
	regs[R_EFLAGS] = istack->eflags;
	PRINT_REGS(regs,"\t");

	/*proc_dbg_printAll();*/
	intrpt_setEnabled(false);
	util_halt();
}

sFuncCall *util_getUserStackTrace(sThread *t,sIntrptStackFrame *stack) {
	return util_getStackTrace((u32*)stack->ebp,
			t->ustackBegin - t->ustackPages * PAGE_SIZE,t->ustackBegin);
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

u32 util_sprintfLen(const char *fmt,...) {
	u32 len;
	va_list ap;
	va_start(ap,fmt);
	len = util_vsprintfLen(fmt,ap);
	va_end(ap);
	return len;
}

u32 util_vsprintfLen(const char *fmt,va_list ap) {
	char c,b;
	char *s;
	u8 pad;
	s32 n;
	u32 u,width;
	bool readFlags;
	u8 base;
	u32 len = 0;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* finished? */
			if(c == '\0') {
				return len;
			}
			len++;
		}

		/* read flags */
		readFlags = true;
		while(readFlags) {
			switch(*fmt) {
				case '-':
				case '0':
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
				width = util_getnwidth(n);
				len += MAX(width,pad);
				break;
			/* unsigned integer */
			case 'b':
			case 'u':
			case 'o':
			case 'x':
				u = va_arg(ap, u32);
				base = c == 'o' ? 8 : (c == 'x' ? 16 : (c == 'b' ? 2 : 10));
				width = util_getuwidth(u,base);
				len += MAX(width,pad);
				break;
			/* string */
			case 's':
				s = va_arg(ap, char*);
				width = strlen(s);
				len += MAX(width,pad);
				break;
			/* character */
			case 'c':
				b = (char)va_arg(ap, u32);
				len++;
				break;
			/* all other */
			default:
				len++;
				break;
		}
	}
}

bool util_sprintf(sStringBuffer *buf,const char *fmt,...) {
	bool res;
	va_list ap;
	va_start(ap,fmt);
	res = util_vsprintf(buf,fmt,ap);
	va_end(ap);
	return res;
}

bool util_vsprintf(sStringBuffer *buf,const char *fmt,va_list ap) {
	char c,b,padchar;
	char *s,*str;
	u8 pad;
	s32 n;
	u32 u,width;
	bool readFlags,padRight;
	u8 base;

	if(buf->dynamic && buf->str == NULL) {
		buf->size = SPRINTF_MIN_INC_SIZE;
		buf->str = (char*)kheap_alloc(SPRINTF_MIN_INC_SIZE * sizeof(char));
		if(!buf->str)
			return false;
	}
	str = buf->str + buf->len;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* finished? */
			if(c == '\0') {
				SPRINTF_ADD_CHAR('\0');
				return true;
			}
			SPRINTF_ADD_CHAR(c);
			buf->len++;
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
				width = util_getnwidth(n);
				SPRINTF_INCREASE(width,pad);
				if(!padRight && pad > 0)
					str += util_sprintfPad(str,padchar,pad - width);
				str += util_sprintn(str,n);
				if(padRight && pad > 0)
					str += util_sprintfPad(str,padchar,pad - width);
				buf->len += MAX(width,pad);
				break;
			/* unsigned integer */
			case 'b':
			case 'u':
			case 'o':
			case 'x':
				u = va_arg(ap, u32);
				base = c == 'o' ? 8 : (c == 'x' ? 16 : (c == 'b' ? 2 : 10));
				width = util_getuwidth(u,base);
				SPRINTF_INCREASE(width,pad);
				if(!padRight && pad > 0)
					str += util_sprintfPad(str,padchar,pad - width);
				str += util_sprintu(str,u,base);
				if(padRight && pad > 0)
					str += util_sprintfPad(str,padchar,pad - width);
				buf->len += MAX(width,pad);
				break;
			/* string */
			case 's':
				s = va_arg(ap, char*);
				width = strlen(s);
				SPRINTF_INCREASE(width,pad);
				if(!padRight && pad > 0)
					str += util_sprintfPad(str,padchar,pad - width);
				while(*s) {
					SPRINTF_ADD_CHAR(*s);
					s++;
				}
				if(padRight && pad > 0)
					str += util_sprintfPad(str,padchar,pad - width);
				buf->len += MAX(width,pad);
				break;
			/* character */
			case 'c':
				b = (char)va_arg(ap, u32);
				SPRINTF_ADD_CHAR(b);
				buf->len++;
				break;
			/* all other */
			default:
				SPRINTF_ADD_CHAR(c);
				buf->len++;
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
	if(n < 0) {
		*str = '-';
		n = -n;
		c++;
	}
	if(n >= 10)
		c += util_sprintn(str + c,n / 10);
	str += c;
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
