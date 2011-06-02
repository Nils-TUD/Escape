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
#include <sys/intrpt.h>

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
		struct {
			void *src;
			uintptr_t dst;
			size_t size;
		} direct;
		struct {
			void **srcPtr;
			uintptr_t result;
			size_t size;
		} ptr;
	} data;
} sVM86Memarea;

typedef struct {
	uint16_t interrupt;
	sVM86Regs regs;
	sVM86Memarea *areas;
	size_t areaCount;
} sVM86Info;

/**
 * Creates a vm86-task
 *
 * @return 0 on success
 */
int vm86_create(void);

/**
 * Performs a VM86-interrupt
 *
 * @param interrupt the interrupt-number
 * @param regs the register
 * @param areas the memareas
 * @param areaCount the number of memareas
 * @return 0 on success
 */
int vm86_int(uint16_t interrupt,sVM86Regs *regs,const sVM86Memarea *areas,size_t areaCount);

/**
 * Handles a GPF
 *
 * @param stack the interrupt-stack-frame
 */
void vm86_handleGPF(sIntrptStackFrame *stack);

#endif /* VM86_H_ */
