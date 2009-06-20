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

#ifndef FPU_H_
#define FPU_H_

#include "common.h"

/* the state of FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 */
typedef struct {
	u16 : 16;		/* reserved */
	u16 cs;
	u32 fpuIp;
	u16 fop;
	u16 ftw;
	u16 fsw;
	u16 fcw;
	u32 mxcsrMask;
	u32 mxcsr;
	u16 : 16;		/* reserved */
	u16 ds;
	u32 fpuDp;
	/* reserved */
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 st0mm0[10];
	/* reserved */
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 st1mm1[10];
	/* reserved */
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 st2mm2[10];
	/* reserved */
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 st3mm3[10];
	/* reserved */
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 st4mm4[10];
	/* reserved */
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 st5mm5[10];
	/* reserved */
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 st6mm6[10];
	/* reserved */
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 : 8;
	u8 st7mm7[10];
	u8 xmm0[16];
	u8 xmm1[16];
	u8 xmm2[16];
	u8 xmm3[16];
	u8 xmm4[16];
	u8 xmm5[16];
	u8 xmm6[16];
	u8 xmm7[16];
	/* reserved */
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
	u32 : 32;
} __attribute__((packed)) sXState;

/* the FPU-state */
typedef struct {
	u16 : 16;
	u16 controlWord;
	u16 : 16;
	u16 statusWord;
	u16 : 16;
	u16 tagWord;
	u32 fpuIPOffset;
	u16 : 16;
	u16 fpuIPSel;
	u32 fpuOpOffset;
	u16 : 16;
	u16 fpuOpSel;
	struct {
		u16 high;
		u32 mid;
		u32 low;
	} __attribute__((packed)) regs[8];
} __attribute__((packed)) sFPUState;

/**
 * Inits the FPU
 */
void fpu_init(void);

/**
 * Locks the FPU so that we'll receive a EX_CO_PROC_NA exception as soon as someone
 * uses a FPU-instruction
 */
void fpu_lockFPU(void);

/**
 * Handles the EX_CO_PROC_NA exception
 *
 * @param state pointer to the state-memory
 */
void fpu_handleCoProcNA(sFPUState **state);

/**
 * Free's the given FPU-state
 *
 * @param state the state
 */
void fpu_freeState(sFPUState **state);

#endif /* FPU_H_ */
