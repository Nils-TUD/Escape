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

#include <esc/common.h>

#define BOOT_PATH_MAX	64
#define BOOT_PROG_MAX	8

/* a program we should load */
struct LoadProg {
	char path[BOOT_PATH_MAX];
	char command[BOOT_PATH_MAX];
	uintptr_t start;
	size_t size;
};

struct BootInfo {
	uint64_t cpuHz;
	size_t progCount;
	const LoadProg *progs;
	size_t memSize;
	size_t diskSize;
	uintptr_t kstackBegin;
	uintptr_t kstackEnd;
};
