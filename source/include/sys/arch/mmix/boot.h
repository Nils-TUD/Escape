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

#define MAX_PATH_LEN	64
#define MAX_CMD_LEN		128
#define MAX_PROG_COUNT	8

#define BL_K_ID			0
#define BL_DISK_ID		1
#define BL_FS_ID		2
#define BL_RTC_ID		3

/* a program we should load */
struct LoadProg {
	char path[MAX_PATH_LEN];
	char command[MAX_PATH_LEN];
	uint id;
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
