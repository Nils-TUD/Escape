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

#include <sys/common.h>
#include <sys/defines.h>
#include <stddef.h>

typedef int vmreg_t;
typedef uintptr_t frameno_t;

#ifndef NDEBUG
#define DEBUGGING 1
#endif

/* indicates that a pointer might point into userspace */
#define USER
/* indicates that a function is called when handling an interrupt */
#define INTRPT

#define INVALID_FRAME	((frameno_t)-1)
