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

#ifndef BOOT_H_
#define BOOT_H_

#include <esc/common.h>
#include <sys/intrpt.h>

#ifdef __i386__
#include <sys/arch/i586/boot.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/boot.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/boot.h>
#endif

/**
 * @return size of the kernel (in bytes)
 */
size_t boot_getKernelSize(void);

/**
 * @return size of the multiboot-modules (in bytes)
 */
size_t boot_getModuleSize(void);

/**
 * @return the usable memory in bytes
 */
size_t boot_getUsableMemCount(void);

/**
 * Loads all multiboot-modules
 *
 * @param stack the interrupt-stack-frame
 */
void boot_loadModules(sIntrptStackFrame *stack);

#if DEBUGGING

/**
 * Prints all interesting elements of the multi-boot-structure
 */
void boot_dbg_print(void);

#endif

#endif /* BOOT_H_ */
