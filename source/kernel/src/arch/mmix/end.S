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

.section .bss
	.align	8

// This must be the last file on the link-line, allocating global registers
// from the top.

// Register $254 is the stack-pointer.
sp GREG

// Register $253 is frame-pointer.  It's not supposed to be used in most
// functions.
fp GREG

// $252 is the static chain register; nested functions receive the
// context of the surrounding function through a pointer passed in this
// register.
static_chain GREG
struct_value_reg GREG

// These registers are used to pass state at an exceptional return (C++).
eh_state_3 GREG
eh_state_2 GREG
eh_state_1 GREG
eh_state_0 GREG

#if defined(__MMIX_ABI_GNU__)

// Allocate global registers used by the GNU ABI.
gnu_parm_reg_16 GREG
gnu_parm_reg_15 GREG
gnu_parm_reg_14 GREG
gnu_parm_reg_13 GREG
gnu_parm_reg_12 GREG
gnu_parm_reg_11 GREG
gnu_parm_reg_10 GREG
gnu_parm_reg_9 GREG
gnu_parm_reg_8 GREG
gnu_parm_reg_7 GREG
gnu_parm_reg_6 GREG
gnu_parm_reg_5 GREG
gnu_parm_reg_4 GREG
gnu_parm_reg_3 GREG
gnu_parm_reg_2 GREG
gnu_parm_reg_1 GREG

#endif /* __MMIX_ABI_GNU__ */
