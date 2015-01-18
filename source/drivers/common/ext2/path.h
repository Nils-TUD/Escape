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

#include "ext2.h"

class Ext2Path {
	Ext2Path() = delete;

public:
	/**
	 * Resolves the given path to the inode-number
	 *
	 * @param e the ext2-handle
	 * @param u the user
	 * @param path the path
	 * @param flags the flags with which to open the file
	 * @return the inode-Number or EXT2_BAD_INO
	 */
	static ino_t resolve(Ext2FileSystem *e,FSUser *u,const char *path,uint flags,mode_t mode);
};
