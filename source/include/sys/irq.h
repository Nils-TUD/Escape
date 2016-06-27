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

#include <sys/common.h>
#include <sys/syscalls.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Creates a new process-local semaphore that is attached to the given IRQ. That is, as soon as the
 * IRQ arrives, the semaphore is up'ed.
 *
 * @param irq the interrupt number to attach it to
 * @param name the name to display for this IRQ
 * @param msiaddr will be set to the address to program into MSI address registers, if <msiaddr> != 0
 * @param msival will be set to the value to program into the MSI data register, if <msiaddr> != 0
 * @return the semaphore id or a negative error-code
 */
A_CHECKRET int semcrtirq(int irq,const char *name,uint64_t *msiaddr,uint32_t *msival);

/**
 * Creates a new process-local semaphore that is attached to the given IRQ. That is, as soon as the
 * IRQ arrives, the semaphore is up'ed.
 *
 * @param fd the file descriptor for the IRQ to attach it to (/sys/irq/<no>)
 * @param name the name to display for this IRQ
 * @param msiaddr will be set to the address to program into MSI address registers, if <msiaddr> != 0
 * @param msival will be set to the value to program into the MSI data register, if <msiaddr> != 0
 * @return the semaphore id or a negative error-code
 */
A_CHECKRET static inline int fsemcrtirq(int fd,const char *name,uint64_t *msiaddr,uint32_t *msival) {
	return syscall4(SYSCALL_SEMCRTIRQ,fd,(ulong)name,(ulong)msiaddr,(ulong)msival);
}

#if defined(__cplusplus)
}
#endif
