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

#ifndef SYSCALLS_OTHER_H_
#define SYSCALLS_OTHER_H_

#include <sys/machine/intrpt.h>

/**
 * Loads the multiboot-modules. This is intended for initloader only!
 */
void sysc_loadMods(sIntrptStackFrame *stack);

/**
 * Temporary syscall to print out a character
 */
void sysc_debugc(sIntrptStackFrame *stack);

/**
 * Just intended for debugging. May be used for anything :)
 * It's just a system-call thats used by nothing else, so we can use it e.g. for printing
 * debugging infos in the kernel to points of time controlled by user-apps.
 */
void sysc_debug(sIntrptStackFrame *stack);

/**
 * Gets the value of a kernel-configuration
 *
 * @param u32 the id of the config-value (CONF_*)
 * @return s32 the value or the negative error-code
 */
void sysc_getConf(sIntrptStackFrame *stack);

/**
 * Aquire a lock; wait until its unlocked, if necessary
 *
 * @param ident the lock-ident
 * @return s32 0 on success
 */
void sysc_lock(sIntrptStackFrame *stack);

/**
 * Releases a lock
 *
 * @param ident the lock-ident
 * @return s32 0 on success
 */
void sysc_unlock(sIntrptStackFrame *stack);

#endif /* SYSCALLS_OTHER_H_ */
