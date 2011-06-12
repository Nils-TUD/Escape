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

#ifndef ECO32_THREAD_H_
#define ECO32_THREAD_H_

#include <esc/common.h>

/* the thread-state which will be saved for context-switching */
typedef struct {
	uint32_t r16;
	uint32_t r17;
	uint32_t r18;
	uint32_t r19;
	uint32_t r20;
	uint32_t r21;
	uint32_t r22;
	uint32_t r23;
	uint32_t r25;
	uint32_t r29;
	uint32_t r30;
	uint32_t r31;
} sThreadRegs;

typedef struct {
	char dummy;	/* empty struct not allowed */
} sThreadArchAttr;

#define STACK_REG_COUNT		1

#endif /* ECO32_THREAD_H_ */
