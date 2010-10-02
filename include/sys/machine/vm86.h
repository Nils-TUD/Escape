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

#include <sys/common.h>
#include <sys/machine/intrpt.h>

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
		struct {
			void *src;
			u32 dst;
			u32 size;
		} direct;
		struct {
			void **srcPtr;
			u32 result;
			u32 size;
		} ptr;
	} data;
} sVM86Memarea;

typedef struct {
	u16 interrupt;
	sVM86Regs regs;
	sVM86Memarea *areas;
	u16 areaCount;
} sVM86Info;

/**
 * Creates a vm86-task
 *
 * @return 0 on success
 */
s32 vm86_create(void);

/**
 * Performs a VM86-interrupt
 *
 * @param interrupt the interrupt-number
 * @param regs the register
 * @param areas the memareas
 * @param areaCount the number of memareas
 * @return 0 on success
 */
s32 vm86_int(u16 interrupt,sVM86Regs *regs,const sVM86Memarea *areas,u16 areaCount);

/**
 * Handles a GPF
 *
 * @param stack the interrupt-stack-frame
 * @return true if handled?? TODO
 */
bool vm86_handleGPF(sIntrptStackFrame *stack);

#endif /* VM86_H_ */
