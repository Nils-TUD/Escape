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
#include "setup.h"

/**
 * Loads the given library with given file-descriptor, including all dependencies
 *
 * @param binFd the file-desc
 * @param dst the library
 */
void load_doLoad(int binFd,sSharedLib *dst);

/**
 * Loads all segments from all libraries into memory
 *
 * @param tlsStart pointer to the tlsStart argument for the program to load
 * @param tlsSize pointer to the tlsSize argument for the program to load
 * @return the entry-point of the executable
 */
uintptr_t load_addSegments(uint *tlsStart,size_t *tlsSize);
