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

#include <esc/common.h>
#include <esc/lock.h>

/**
 * Assembler-routines
 */
extern s32 _lock(u32 ident,bool global,u16 flags);
extern s32 _unlock(u32 ident,bool global);

s32 lock(u32 ident,u16 flags) {
	/* nasm doesn't like "lock" as label... */
	return _lock(ident,false,flags);
}

void locku(tULock *l) {
	__asm__ (
		"mov $1,%%ecx;"				/* ecx=1 to lock it for others */
		"lockuLoop:"
		"	xor	%%eax,%%eax;"		/* clear eax */
		"	lock;"					/* lock next instruction */
		"	cmpxchg %%ecx,(%0);"	/* compare ecx with eax; if equal exchange ecx with l */
		"	jnz		lockuLoop;"		/* try again if not equal */
		: : "D" (l)
	);
}

s32 lockg(u32 ident,u16 flags) {
	return _lock(ident,true,flags);
}

s32 unlock(u32 ident) {
	return _unlock(ident,false);
}

void unlocku(tULock *l) {
	*l = false;
}

s32 unlockg(u32 ident) {
	return _unlock(ident,true);
}
