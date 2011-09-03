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

#ifndef FPU_H_
#define FPU_H_

#include <sys/common.h>

/* the state of FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 */
typedef struct {
	uint16_t : 16;		/* reserved */
	uint16_t cs;
	uint32_t fpuIp;
	uint16_t fop;
	uint16_t ftw;
	uint16_t fsw;
	uint16_t fcw;
	uint32_t mxcsrMask;
	uint32_t mxcsr;
	uint16_t : 16;		/* reserved */
	uint16_t ds;
	uint32_t fpuDp;
	/* reserved */
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t st0mm0[10];
	/* reserved */
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t st1mm1[10];
	/* reserved */
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t st2mm2[10];
	/* reserved */
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t st3mm3[10];
	/* reserved */
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t st4mm4[10];
	/* reserved */
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t st5mm5[10];
	/* reserved */
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t st6mm6[10];
	/* reserved */
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t : 8;
	uint8_t st7mm7[10];
	uint8_t xmm0[16];
	uint8_t xmm1[16];
	uint8_t xmm2[16];
	uint8_t xmm3[16];
	uint8_t xmm4[16];
	uint8_t xmm5[16];
	uint8_t xmm6[16];
	uint8_t xmm7[16];
	/* reserved */
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
	uint32_t : 32;
} A_PACKED sXState;

/* the FPU-state */
typedef struct {
	uint16_t : 16;
	uint16_t controlWord;
	uint16_t : 16;
	uint16_t statusWord;
	uint16_t : 16;
	uint16_t tagWord;
	uint32_t fpuIPOffset;
	uint16_t : 16;
	uint16_t fpuIPSel;
	uint32_t fpuOpOffset;
	uint16_t : 16;
	uint16_t fpuOpSel;
	struct {
		uint16_t high;
		uint32_t mid;
		uint32_t low;
	} A_PACKED regs[8];
} A_PACKED sFPUState;

/**
 * Performs basic initialization of the FPU (finit, ...)
 */
void fpu_preinit(void);

/**
 * Inits the FPU for usage of processes
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
 * Clones the FPU-state <src> into <dst>
 *
 * @param dst the destination-fpu-state
 * @param src the source-fpu-state
 */
void fpu_cloneState(sFPUState **dst,const sFPUState *src);

/**
 * Free's the given FPU-state
 *
 * @param state the state
 */
void fpu_freeState(sFPUState **state);

#endif /* FPU_H_ */
