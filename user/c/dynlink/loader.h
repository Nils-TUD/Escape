/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#ifndef LOADER_H_
#define LOADER_H_

#include <esc/common.h>
#include "setup.h"

/**
 * Loads the given library with given file-descriptor, including all dependencies
 *
 * @param binFd the file-desc
 * @param dst the library
 */
void load_doLoad(tFD binFd,sSharedLib *dst);

/**
 * Loads all segments from all libraries into memory
 *
 * @return the entry-point of the executable
 */
u32 load_addSegments(void);

#endif /* LOADER_H_ */
