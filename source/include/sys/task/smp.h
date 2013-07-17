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

#pragma once

#include <sys/common.h>
#include <sys/task/thread.h>
#include <esc/sllist.h>

/* the IPIs we can send */
#define IPI_WORK			49
#define IPI_TERM			50
#define IPI_FLUSH_TLB		51
#define IPI_WAIT			52
#define IPI_HALT			53
#define IPI_FLUSH_TLB_ACK	54

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
	uint64_t curCycles;
	uint64_t lastCycles;
	uint64_t lastTotal;
	uint64_t lastUpdate;
	Thread *thread;
} sCPU;

/**
 * Inits the SMP-module
 */
void smp_init(void);

/**
 * Architecture-specific initialization. Don't call it!
 *
 * @return true on success
 */
bool smp_init_arch(void);

/**
 * Disables SMP. This is possible even after some CPUs have been added.
 */
void smp_disable(void);

/**
 * @return whether SMP is enabled, i.e. there are multiple CPUs/cores
 */
bool smp_isEnabled(void);

/**
 * Adds the given CPU to the CPU-list
 *
 * @param bootstrap whether its the bootstrap CPU
 * @param id its CPU-id
 * @param ready whether its ready
 */
void smp_addCPU(bool bootstrap,uint8_t id,uint8_t ready);

/**
 * Sets CPU with id <id> to ready
 *
 * @param id the CPU-id
 */
void smp_setReady(cpuid_t id);

/**
 * Should be called if thread <new> is scheduled on CPU <id>
 *
 * @param id the CPU-id
 * @param n the thread to run
 * @param timestamp the current timestamp
 */
void smp_schedule(cpuid_t id,Thread *n,uint64_t timestamp);

/**
 * Updates the runtimes of all CPUs
 */
void smp_updateRuntimes(void);

/**
 * Pauses all other CPUs until smp_resumeOthers() is called.
 */
void smp_pauseOthers(void);

/**
 * Makes all other CPUs resume their computation
 */
void smp_resumeOthers(void);

/**
 * Halts all other CPUs
 */
void smp_haltOthers(void);

/**
 * Tells all other CPUs to flush their TLB and waits until they have done that
 */
void smp_ensureTLBFlushed(void);

/**
 * Sends an IPI to the CPU that executes it currently, if there is any.
 *
 * @param t the thread
 */
void smp_killThread(Thread *t);

/**
 * Wakes up another CPU, so that it can run a thread
 */
void smp_wakeupCPU(void);

/**
 * If there is any CPU that uses the given pagedir, it is flushed
 *
 * @param pdir the pagedir
 */
void smp_flushTLB(pagedir_t *pdir);

/**
 * Sends the IPI <vector> to the CPU <id>
 *
 * @param id the CPU-id
 * @param vector the IPI-number
 */
void smp_sendIPI(cpuid_t id,uint8_t vector);

/**
 * Renames CPU <old> to CPU <new>
 *
 * @param old the old CPU-id
 * @param newid the new CPU-id
 */
void smp_setId(cpuid_t old,cpuid_t newid);

/**
 * Starts the SMP-system, i.e. the other CPUs
 */
void smp_start(void);

/**
 * @return true if we're executing on the BSP
 */
bool smp_isBSP(void);

/**
 * @return the id of the current CPU
 */
cpuid_t smp_getCurId(void);

/**
 * @return the CPU count
 */
size_t smp_getCPUCount(void);

/**
 * @return the array of CPUs
 */
sCPU **smp_getCPUs(void);

/**
 * Prints all CPUs
 */
void smp_print(void);
