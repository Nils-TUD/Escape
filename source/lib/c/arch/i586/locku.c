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

#include <esc/common.h>
#include <esc/thread.h>
#include <esc/conf.h>

typedef void (*fConstr)(void);

static void initLock(void);

/* actually this isn't optimal because the stdio-constructor may run first and uses locku. but it
 * isn't that bad because at this stage there won't be multiple threads anyway so that the lock
 * is never hold and the number of cpus doesn't matter */
fConstr lockConstr[1] A_INIT = {
	initLock
};
static size_t cpuCount;

static void initLock(void) {
	cpuCount = getConf(CONF_CPU_COUNT) > 1;
}

void locku(tULock *l) {
	__asm__ (
		"mov $1,%%ecx;"				/* ecx=1 to lock it for others */
		"lockuLoop:"
		"	xor	%%eax,%%eax;"		/* clear eax */
		"	lock;"					/* lock next instruction */
		"	cmpxchg %%ecx,(%0);"	/* compare l with eax; if equal exchange ecx with l */
		"	jz		done;"
		"	mov		%1,%%eax;"
		"	test	%%eax,%%eax;"
		"	jnz		lockuLoop;"		/* if there are multiple cpus, just sit here and wait */
		"	call	yield;"			/* with only one cpu, waiting makes no sense because nobody
									   else can release the lock */
		"	jmp		lockuLoop;"		/* try again */
		"done:"
		/* outputs */
		:
		/* inputs */
		: "D" (l), "m" (cpuCount)
		/* eax, ecx and cc will be clobbered; we need memory as well because *l is changed */
		: "eax", "ecx", "cc", "memory"
	);
}

void unlocku(tULock *l) {
	*l = false;
}
