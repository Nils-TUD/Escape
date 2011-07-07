/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#ifndef SYSCALLS_MEM_H_
#define SYSCALLS_MEM_H_

#include <sys/intrpt.h>

/* protection-flags */
#define PROT_READ			1
#define PROT_WRITE			2

int sysc_changeSize(sIntrptStackFrame *stack);
int sysc_addRegion(sIntrptStackFrame *stack);
int sysc_setRegProt(sIntrptStackFrame *stack);
int sysc_mapPhysical(sIntrptStackFrame *stack);
int sysc_createSharedMem(sIntrptStackFrame *stack);
int sysc_joinSharedMem(sIntrptStackFrame *stack);
int sysc_leaveSharedMem(sIntrptStackFrame *stack);
int sysc_destroySharedMem(sIntrptStackFrame *stack);

#endif /* SYSCALLS_MEM_H_ */
