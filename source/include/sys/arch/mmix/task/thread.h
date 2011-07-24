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

#ifndef MMIX_THREAD_H_
#define MMIX_THREAD_H_

#include <esc/common.h>

/* the thread-state which will be saved for context-switching */
typedef struct {
	uintptr_t stackEnd;
} sThreadRegs;

typedef struct {
	uint64_t rbb;
	uint64_t rww;
	uint64_t rxx;
	uint64_t ryy;
	uint64_t rzz;
} sKSpecRegs;

typedef struct {
	/* use as a temporary kernel-stack for cloning */
	frameno_t tempStack;
	/* when handling a signal, we have to backup these registers */
	sKSpecRegs specRegLevels[MAX_INTRPT_LEVELS];
} sThreadArchAttr;

#define STACK_REG_COUNT		2

sKSpecRegs *thread_getSpecRegs(void);
void thread_pushSpecRegs(void);
void thread_popSpecRegs(void);

#endif /* MMIX_THREAD_H_ */
