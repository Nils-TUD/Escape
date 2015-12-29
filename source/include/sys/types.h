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

/* for mmix: ensure that the stdint.h provided by gcc includes our stdint.h */
#define __STDC_HOSTED__ 1
#include <stdint.h>
#undef __STDC_HOSTED__

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

typedef int32_t ino_t;
typedef uint32_t block_t;
typedef int32_t dev_t;
typedef ushort nlink_t;
typedef ushort blksize_t;
typedef uint blkcnt_t;

typedef uint8_t cpuid_t;

typedef uint32_t msgid_t;
typedef int errcode_t;

typedef uintptr_t evobj_t;

typedef uint32_t time_t;

typedef size_t gsize_t;
typedef int gpos_t;
typedef uchar gcoldepth_t;
typedef ushort gwinid_t;
