/**
 * $Id: setjmp.h 634 2010-05-01 12:20:20Z nasmussen $
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
	u32 ebx;
	u32 esp;
	u32 edi;
	u32 esi;
	u32 ebp;
	u32 eflags;
	u32 eip;
} sJumpEnv;

extern s32 setjmp(sJumpEnv *env);
extern s32 longjmp(sJumpEnv *env,s32 val);

#endif /* SETJMP_H_ */
