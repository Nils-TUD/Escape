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

#include "iso9660.h"

class ISO9660Dir {
	ISO9660Dir() = delete;

public:
	/**
	 * Resolves the given path via the directories to the file-id (LBA * blockSize + offset)
	 *
	 * @param h the iso9660-handle
	 * @param u the user
	 * @param path the path
	 * @param flags the flags with which to open the file
	 * @return the id or < 0
	 */
	static ino_t resolve(ISO9660FileSystem *h,FSUser *u,const char *path,uint flags);

private:
	/**
	 * calcuates an imaginary inode-number from block-number and offset in the directory-entries
	 */
	static ino_t getIno(ulong blockNo,size_t blockSize,size_t offset) {
		return blockNo * blockSize + offset;
	}

	/**
	 * Checks whether <user> matches <disk>
	 */
	static bool match(const char *user,const char *disk,size_t userLen,size_t diskLen);
};
