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
#include <fs/ext2/ext2.h>

class Ext2FileSystem;

class Ext2SBMng {
public:
	/**
	 * Inits the super-block; reads it from disk and reads the block-group-descriptor-table
	 *
	 * @param e the ext2-data
	 * @return 0 on success
	 */
	explicit Ext2SBMng(Ext2FileSystem *e);

	/**
	 * @return the superblock
	 */
	Ext2SuperBlock *get() {
		return &_superBlock;
	}
	const Ext2SuperBlock *get() const {
		return &_superBlock;
	}

	/**
	 * Marks the superblock as dirty
	 */
	void markDirty() {
		_sbDirty = true;
	}

	/**
	 * Updates the super-block, if it is dirty
	 *
	 * @param e the ext2-data
	 */
	void update();

private:
	bool _sbDirty;
	Ext2SuperBlock _superBlock;
	Ext2FileSystem *_fs;
};
