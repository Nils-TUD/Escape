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
#include <sys/task/thread.h>
#include <esc/sllist.h>

#define IPI_WORK			0x31
#define IPI_TERM			0x32
#define IPI_FLUSH_TLB		0x33
#define IPI_WAIT			0x34
#define IPI_HALT			0x35
#define IPI_FLUSH_TLB_ACK	0x36

#ifdef __i386__
#include <sys/arch/i586/task/smp.h>
#endif

typedef struct {
	uint8_t id;
	uint8_t bootstrap;
	uint8_t ready;
	size_t schedCount;
	uint64_t runtime;
	uint64_t lastSched;
	sThread *thread;
} sCPU;

void smp_init(void);
bool smp_init_arch(void);
bool smp_isEnabled(void);
void smp_addCPU(bool bootstrap,uint8_t id,uint8_t ready);
void smp_setReady(cpuid_t id);
void smp_schedule(cpuid_t id,sThread *new,uint64_t timestamp);
void smp_pauseOthers(void);
void smp_resumeOthers(void);
void smp_haltOthers(void);
void smp_ensureTLBFlushed(void);
void smp_killThread(sThread *t);
void smp_wakeupCPU(void);
void smp_flushTLB(tPageDir *pdir);
void smp_sendIPI(cpuid_t id,uint8_t vector);
void smp_setId(cpuid_t old,cpuid_t new);
void smp_start(void);
bool smp_isBSP(void);
cpuid_t smp_getCurId(void);
size_t smp_getCPUCount(void);
sCPU **smp_getCPUs(void);
void smp_print(void);

#endif /* SMP_H_ */
