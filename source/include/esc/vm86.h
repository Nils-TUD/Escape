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

#ifndef VM86_H_
#define VM86_H_

#include <esc/common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VM86_MEM_DIRECT		0
#define VM86_MEM_PTR		1

typedef struct {
	uint16_t ax;
	uint16_t bx;
	uint16_t cx;
	uint16_t dx;
	uint16_t si;
	uint16_t di;
	uint16_t ds;
	uint16_t es;
} sVM86Regs;

typedef struct {
	uchar type;
	union {
		/* copy it directly to a place in the process-space (dst) */
		struct {
			void *src;
			uintptr_t dst;
			size_t size;
		} direct;
		/* copy result from *<srcPtr> (a pointer to a real-mode-address) to the process space */
		struct {
			void **srcPtr;
			uintptr_t result;
			size_t size;
		} ptr;
	} data;
} sVM86Memarea;

#define VM86_PAGE_SIZE		4096
#define VM86_ADDR(src,dst)	(((uintptr_t)(dst) & ~(VM86_PAGE_SIZE - 1)) | \
		((uintptr_t)(src) & (VM86_PAGE_SIZE - 1)))
#define VM86_OFF(src,dst)	(VM86_ADDR(src,dst) & 0xFFFF)
#define VM86_SEG(src,dst)	((VM86_ADDR(src,dst) & 0xF0000) >> 4)

/**
 * Performs a VM86-interrupt. That means a VM86-task is created as a child-process, the
 * registers are set correspondingly and the tasks starts at the handler for the given interrupt.
 * As soon as the interrupt is finished the result is copied into the registers.
 * You can also specify memory-areas which will be mapped at the requested real-mode-address,
 * so that it can be read to it and written from it.
 *
 * @param regs the registers
 * @param areas the memareas (may be NULL)
 * @param areaCount mem-area count
 * @return 0 on success
 */
int vm86int(uint16_t interrupt,sVM86Regs *regs,sVM86Memarea *areas,size_t areaCount);

#ifdef __cplusplus
}
#endif

#endif /* VM86_H_ */
