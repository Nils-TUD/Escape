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
#include <sys/interrupts.h>

typedef void (*boottask_func)(void);
struct BootTask {
	const char *name;
	boottask_func execute;
};

struct BootTaskList {
	const BootTask *tasks;
	size_t count;
	size_t moduleCount;

	explicit BootTaskList(const BootTask *tasks,size_t count)
		: tasks(tasks), count(count), moduleCount(0) {
	}
};

#ifdef __i386__
#include <sys/arch/i586/boot.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/boot.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/boot.h>
#endif

class Boot {
	Boot() = delete;

public:
	/**
	 * Starts the boot-process
	 *
	 * @param info the boot-information
	 */
	static void start(BootInfo *info);

	/**
	 * @return the multiboot-info-structure
	 */
	static const BootInfo *getInfo();

	/**
	 * Displays the given text that should indicate that a task in the boot-process
	 * has just been started
	 *
	 * @param text the text to display
	 */
	static void taskStarted(const char *text);

	/**
	 * Finishes an item, i.e. updates the progress-bar
	 */
	static void taskFinished();

	/**
	 * Parses the given line into arguments
	 *
	 * @param line the line to parse
	 * @param argc will be set to the number of found arguments
	 * @return the arguments (statically allocated)
	 */
	static const char **parseArgs(const char *line,int *argc);

	/**
	 * @return size of the kernel (in bytes)
	 */
	static size_t getKernelSize();

	/**
	 * @return size of the multiboot-modules (in bytes)
	 */
	static size_t getModuleSize();

	/**
	 * Determines the physical address range of the multiboot-module with given name
	 *
	 * @param name the module-name
	 * @param size will be set to the size in bytes
	 * @return the address of the module (physical) or 0 if not found
	 */
	static uintptr_t getModuleRange(const char *name,size_t *size);

	/**
	 * Loads all multiboot-modules
	 *
	 * @param stack the interrupt-stack-frame
	 */
	static int loadModules(IntrptStackFrame *stack);

	/**
	 * Prints all interesting elements of the multi-boot-structure
	 */
	static void print(OStream &os);

private:
	static void archStart(BootInfo *info);
	static void drawProgressBar();

	/**
	 * The boot-tasks to load
	 */
	static BootTaskList taskList;
	static bool loadedMods;
};
