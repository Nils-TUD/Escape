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

#include <sys/common.h>
#include <sys/mem/kheap.h>
#include <sys/task/smp.h>
#include <sys/cpu.h>
#include <sys/printf.h>
#include <sys/video.h>
#include <string.h>

const char *CPU::specialRegs[] = {
	/* 00 */	"rB",
	/* 01 */	"rD",
	/* 02 */	"rE",
	/* 03 */	"rH",
	/* 04 */	"rJ",
	/* 05 */	"rM",
	/* 06 */	"rR",
	/* 07 */	"rBB",
	/* 08 */	"rC",
	/* 09 */	"rN",
	/* 0A */	"rO",
	/* 0B */	"rS",
	/* 0C */	"rI",
	/* 0D */	"rT",
	/* 0E */	"rTT",
	/* 0F */	"rK",
	/* 10 */	"rQ",
	/* 11 */	"rU",
	/* 12 */	"rV",
	/* 13 */	"rG",
	/* 14 */	"rL",
	/* 15 */	"rA",
	/* 16 */	"rF",
	/* 17 */	"rP",
	/* 18 */	"rW",
	/* 19 */	"rX",
	/* 1A */	"rY",
	/* 1B */	"rZ",
	/* 1C */	"rWW",
	/* 1D */	"rXX",
	/* 1E */	"rYY",
	/* 1F */	"rZZ",
	/* 20 */	"rSS"
};

uint64_t CPU::cpuHz;

void CPUBase::sprintf(sStringBuffer *buf) {
	uint64_t rn = CPU::getSpecial(rN);
	const SMP::CPU *smpcpu = SMP::getCPUs()[0];
	prf_sprintf(buf,"CPU 0:\n");
	prf_sprintf(buf,"\t%-12s%lu Cycles\n","Total:",smpcpu->lastTotal);
	prf_sprintf(buf,"\t%-12s%Lu Cycles\n","Non-Idle:",smpcpu->lastCycles);
	prf_sprintf(buf,"\t%-12s%lu Hz\n","Speed:",CPU::getSpeed());
	prf_sprintf(buf,"\t%-12s%s\n","Vendor:","THM");
	prf_sprintf(buf,"\t%-12s%s\n","Model:","GIMMIX");
	prf_sprintf(buf,"\t%-12s%d.%d.%d\n","Version:",rn >> 56,(rn >> 48) & 0xFF,(rn >> 40) & 0xFF);
	prf_sprintf(buf,"\t%-12s%lu\n","Builddate",rn & 0xFFFFFFFFFF);
}
