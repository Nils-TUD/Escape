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

#include <esc/common.h>
#include <sys/intrpt.h>

typedef void (*fBootTask)(void);
typedef struct {
	const char *name;
	fBootTask execute;
} sBootTask;

typedef struct {
	const sBootTask *tasks;
	size_t count;
	size_t moduleCount;
} sBootTaskList;

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
 * Starts the boot-process
 *
 * @param info the boot-information
 */
void boot_start(sBootInfo *info);

/**
 * Displays the given text that should indicate that a task in the boot-process
 * has just been started
 *
 * @param text the text to display
 */
void boot_taskStarted(const char *text);

/**
 * Finishes an item, i.e. updates the progress-bar
 */
void boot_taskFinished(void);

/**
 * Parses the given line into arguments
 *
 * @param line the line to parse
 * @param argc will be set to the number of found arguments
 * @return the arguments (statically allocated)
 */
const char **boot_parseArgs(const char *line,int *argc);

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
 * Determines the physical address range of the multiboot-module with given name
 *
 * @param name the module-name
 * @param size will be set to the size in bytes
 * @return the address of the module (physical) or 0 if not found
 */
uintptr_t boot_getModuleRange(const char *name,size_t *size);

/**
 * Loads all multiboot-modules
 *
 * @param stack the interrupt-stack-frame
 */
int boot_loadModules(sIntrptStackFrame *stack);

/**
 * Prints all interesting elements of the multi-boot-structure
 */
void boot_print(void);
