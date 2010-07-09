/**
 * $Id: vm86.h 578 2010-03-29 15:54:22Z nasmussen $
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

#ifndef VM86_H_
#define VM86_H_

#include <esc/common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VM86_MEM_DIRECT		0
#define VM86_MEM_PTR		1

typedef struct {
	u16 ax;
	u16 bx;
	u16 cx;
	u16 dx;
	u16 si;
	u16 di;
	u16 ds;
	u16 es;
} sVM86Regs;

typedef struct {
	u8 type;
	union {
		/* copy it directly to a place in the process-space (dst) */
		struct {
			void *src;
			u32 dst;
			u32 size;
		} direct;
		/* copy result from *<srcPtr> (a pointer to a real-mode-address) to the process space */
		struct {
			void **srcPtr;
			u32 result;
			u32 size;
		} ptr;
	} data;
} sVM86Memarea;

#define VM86_PAGE_SIZE		4096
#define VM86_ADDR(src,dst)	(((u32)(dst) & ~(VM86_PAGE_SIZE - 1)) | ((u32)(src) & (VM86_PAGE_SIZE - 1)))
#define VM86_OFF(src,dst)	(VM86_ADDR(src,dst) & 0xFFFF)
#define VM86_SEG(src,dst)	((VM86_ADDR(src,dst) & 0xF0000) >> 4)

/**
 * Performs a VM86-interrupt. That means a VM86-task is created as a child-process, the
 * registers are set correspondingly and the tasks starts at the handler for the given interrupt.
 * As soon as the interrupt is finished the result is copied into the registers.
 * You can also specify memory-areas which will be mapped at the requested real-mode-address,
 * so that it can be read to it and written from it.
 *
 * @param sVM86Regs* the registers
 * @param sVM86Memarea* the memareas (may be NULL)
 * @param u16 mem-area count
 * @return 0 on success
 */
s32 vm86int(u16 interrupt,sVM86Regs *regs,sVM86Memarea *areas,u16 areaCount);

#ifdef __cplusplus
}
#endif

#endif /* VM86_H_ */
