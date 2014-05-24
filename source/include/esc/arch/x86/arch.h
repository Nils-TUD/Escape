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

#if defined(__i586__)
#	include <esc/arch/i586/arch.h>
#else
#	include <esc/arch/x86_64/arch.h>
#endif

/* segments numbers */
#define SEG_KCODE	1
#define SEG_KDATA	2
#define SEG_UCODE	3
#define SEG_UDATA	4
#define SEG_UCODE2	5
#define SEG_TLS		6
#define SEG_THREAD	7
#define SEG_TSS		8

/* writes the value of the register with given name to c */
#define GET_REG(name,c) \
	__asm__ volatile ( \
		"mov %%" EXPAND(REG(name)) ",%0" \
		: "=a" (c) \
	)
