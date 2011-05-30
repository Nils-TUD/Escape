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

#ifndef SETJMP_H_
#define SETJMP_H_

#include <esc/common.h>

typedef struct {
	uint32_t ebx;
	uint32_t esp;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t eflags;
	uint32_t eip;
} sJumpEnv;

extern int setjmp(sJumpEnv *env);
extern int longjmp(sJumpEnv *env,int val);

#endif /* SETJMP_H_ */
