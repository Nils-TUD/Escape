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

#include <sys/boot.h>
#include <common.h>
#include <interrupts.h>

class Boot {
	Boot() = delete;

public:
	struct Module {
		uintptr_t phys;
		uintptr_t virt;
		size_t size;
		char *name;
	};

	struct MemMap {
		enum {
			MEM_AVAILABLE = 1
		};

		uint32_t size;
		uint64_t baseAddr;
		int type;
	};

	struct Info {
		char *cmdLine;
		size_t modCount;
		Module *mods;
		size_t mmapCount;
		MemMap *mmap;
	};

	/**
	 * Starts the boot-process
	 *
	 * @param info the boot-information (architecture specific)
	 */
	static void start(void *info);

	/**
	 * @return the multiboot-info-structure
	 */
	static const Info *getInfo() {
		return &info;
	}

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
	 * Parses the arch-specific boot information into the general one.
	 */
	static void parseBootInfo();

	/**
	 * Parses the command line into the config-module.
	 */
	static void parseCmdline();

	/**
	 * Creates the boot module files
	 */
	static void createModFiles();

	/**
	 * @return size of the kernel (in bytes)
	 */
	static size_t getKernelSize();

	/**
	 * @return size of the multiboot-modules (in bytes)
	 */
	static size_t getModuleSize();

	/**
	 * @return beginning/end of the module list
	 */
	static Module *modsBegin() {
		return info.mods;
	}
	static Module *modsEnd() {
		return info.mods + info.modCount;
	}

	/**
	 * Determines the physical address range of the multiboot-module with given name
	 *
	 * @param name the module-name
	 * @param size will be set to the size in bytes
	 * @return the address of the module (physical) or 0 if not found
	 */
	static uintptr_t getModuleRange(const char *name,size_t *size);

	/**
	 * Performs initialization tasks.
	 *
	 * @param stack the interrupt-stack-frame
	 */
	static int init(IntrptStackFrame *stack);

	/**
	 * Remembers that we should perform the unittests instead of booting
	 */
	static void setUnittests(void (*handler)()) {
		unittests = handler;
	}

	/**
	 * Prints all interesting elements of the multi-boot-structure
	 */
	static void print(OStream &os);

private:
	static void archStart(void *info);
	static void drawProgressBar();

	/**
	 * The boot-tasks to load
	 */
	static BootTaskList taskList;
	static Info info;
	static void (*unittests)();
};
