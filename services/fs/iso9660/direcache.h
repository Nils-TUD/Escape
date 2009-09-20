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

#ifndef DIRECACHE_H_
#define DIRECACHE_H_

#include <esc/common.h>
#include "iso9660.h"

/**
 * Inits the dir-entry-cache
 *
 * @param h the iso9660 handle
 */
void iso_direc_init(sISO9660 *h);

/**
 * Retrieves the directory-entry with given id (LBA * blockSize + offset in directory)
 *
 * @param h the iso9660 handle
 * @param id the id
 * @return the cached directory-entry or NULL if failed
 */
sISOCDirEntry *iso_direc_get(sISO9660 *h,tInodeNo id);

#endif /* DIRECACHE_H_ */
