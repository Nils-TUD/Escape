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

#include <stddef.h>

#ifndef __cplusplus
typedef enum {false = 0, true = 1} bool;
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef signed long long llong;
typedef unsigned long long ullong;
typedef signed long ssize_t;

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long long uint64_t;
typedef signed long long int64_t;

typedef signed long intptr_t;
typedef unsigned long uintptr_t;

typedef signed long off_t;
typedef uint16_t mode_t;

/* union to access qwords as 2 dwords */
typedef union {
	struct {
		uint32_t lower;
		uint32_t upper;
	} val32;
	uint64_t val64;
} uLongLong;

typedef uint16_t pid_t;
typedef uint16_t tid_t;
typedef uint16_t uid_t;
typedef uint16_t gid_t;

typedef int32_t inode_t;
typedef uint32_t block_t;
typedef int32_t dev_t;

typedef uint8_t cpuid_t;

typedef uint32_t msgid_t;

typedef uintptr_t evobj_t;

typedef uint32_t time_t;

typedef size_t gsize_t;
typedef int gpos_t;
typedef uchar gcoldepth_t;
typedef ushort gwinid_t;
