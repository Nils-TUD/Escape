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

#ifndef SMP_H_
#define SMP_H_

#include <sys/common.h>
#include <esc/sllist.h>

#ifdef __i386__
#include <sys/arch/i586/task/smp.h>
#endif

typedef struct {
	uint8_t id;
	uint8_t bootstrap;
} sCPU;

void smp_init(void);
bool smp_init_arch(void);
bool smp_isEnabled(void);
void smp_addCPU(bool bootstrap,uint8_t id);
void smp_start(void);
cpuid_t smp_getBSPId(void);
bool smp_isBSP(void);
cpuid_t smp_getCurId(void);
size_t smp_getCPUCount(void);
const sSLList *smp_getCPUs(void);

#endif /* SMP_H_ */
