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

#ifndef SYS_COMMON_H_
#define SYS_COMMON_H_

#include <esc/common.h>
#include <esc/defines.h>
#include <stddef.h>

/* file-number (in global file table) */
typedef int file_t;
typedef int vmreg_t;
typedef uintptr_t frameno_t;

#ifndef NDEBUG
#define DEBUGGING 1
#endif

#define K						1024
#define M						(1024 * K)
#define G						(1024 * M)

#endif /*SYS_COMMON_H_*/
