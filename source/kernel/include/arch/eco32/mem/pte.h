/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#pragma once

#define PT_LEVELS				2
#define PT_BPL					10
#define PT_BITS					32
#define PT_ENTRY_COUNT			(PAGE_SIZE >> 2)

/* pte fields */
#define PTE_PRESENT				(1UL << 0)
#define PTE_WRITABLE			(1UL << 1)
#define PTE_NOTSUPER			0
#define PTE_LARGE				0
#define PTE_GLOBAL				0
#define PTE_EXISTS				(1UL << 2)
#define PTE_NO_EXEC				0
#define PTE_FRAMENO(pte)		(((pte) >> PAGE_BITS) & ((1ULL << PT_BITS) - 1))
#define PTE_FRAMENO_MASK		(((1ULL << PT_BITS) - 1) << PAGE_BITS)

#if defined(__cplusplus)
typedef unsigned long pte_t;
#endif
