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

#ifndef REGISTER_H_
#define REGISTER_H_

#include <esc/common.h>
#ifdef IN_KERNEL
#	include <sys/video.h>
#	define rprintf	vid_printf
#else
#	include <stdio.h>
#	define rprintf	printf
#endif

/* the register-offsets used by the macro GET_REGS() */
#define R_EAX		0
#define R_EBX		1
#define R_ECX		2
#define R_EDX		3
#define R_ESI		4
#define R_EDI		5
#define R_ESP		6
#define R_EBP		7
#define R_CS		8
#define R_DS		9
#define R_ES		10
#define R_FS		11
#define R_GS		12
#define R_SS		13
#define R_EFLAGS	14
/* number of registers GET_REGS and PRINT_REGS will need */
#define REG_COUNT	15

/* prints the values in the general-purpose-, segment- and eflags-registers */
#define PRINT_CURREGS(pad) \
	uint32_t ___regs[REG_COUNT]; \
	GET_REGS(___regs); \
	PRINT_REGS(___regs,pad);

/* prints the values of the given registers. expects all above defined registers
 * at the indices R_*. */
#define PRINT_REGS(regs,pad) \
	rprintf(pad "eax=0x%08x, ebx=0x%08x, ecx=0x%08x, edx=0x%08x\n", \
			regs[R_EAX],regs[R_EBX],regs[R_ECX],regs[R_EDX]); \
	rprintf(pad "esi=0x%08x, edi=0x%08x, esp=0x%08x, ebp=0x%08x\n", \
			regs[R_ESI],regs[R_EDI],regs[R_ESP],regs[R_EBP]); \
	rprintf(pad "cs=0x%02x, ds=0x%02x, es=0x%02x, fs=0x%02x, gs=0x%02x, ss=0x%02x\n", \
			regs[R_CS],regs[R_DS],regs[R_ES],regs[R_FS],regs[R_GS],regs[R_SS]); \
	rprintf(pad "eflags=0x%08x\n",regs[R_EFLAGS]);

/* writes the register-values in the given buffer */
#define GET_REGS(buffer) \
	GET_REG("eax",buffer[R_EAX]); \
	GET_REG("ebx",buffer[R_EBX]); \
	GET_REG("ecx",buffer[R_ECX]); \
	GET_REG("edx",buffer[R_EDX]); \
	GET_REG("esi",buffer[R_ESI]); \
	GET_REG("edi",buffer[R_EDI]); \
	GET_REG("esp",buffer[R_ESP]); \
	GET_REG("ebp",buffer[R_EBP]); \
	GET_REG("cs",buffer[R_CS]); \
	GET_REG("ds",buffer[R_DS]); \
	GET_REG("es",buffer[R_ES]); \
	GET_REG("fs",buffer[R_FS]); \
	GET_REG("gs",buffer[R_GS]); \
	GET_REG("ss",buffer[R_SS]); \
	__asm__ ( \
		"pushf;" \
		"pop %%eax" \
		: "=a" (buffer[R_EFLAGS]) \
	);

/* writes the value of the register with given name to c */
#define GET_REG(name,c) \
	__asm__ ( \
		"mov %%" name ",%0" \
		: "=a" (c) \
	);

#endif /* REGISTER_H_ */
