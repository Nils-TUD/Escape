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

#define STRING(x)		#x
#define EXPAND(x)		STRING(x)

#if defined(__i586__)
#	define REG(x)		e##x
#else
#	define REG(x)		r##x
#endif

/* writes the value of the register with given name to c */
#define GET_REG(name,c) \
	__asm__ volatile ( \
		"mov %%" EXPAND(REG(name)) ",%0" \
		: "=a" (c) \
	)
