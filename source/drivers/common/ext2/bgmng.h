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
#include <stdlib.h>

class Ext2FileSystem;

class Ext2BGMng {
public:
	/**
	 * Inits the block-groups, i.e. reads them from disk and stores them
	 *
	 * @param fs the fs-instance
	 * @return 0 on success
	 */
	explicit Ext2BGMng(Ext2FileSystem *fs);

	/**
	 * Destroys the blockgroups
	 */
	~Ext2BGMng() {
		free(_groups);
	}

	/**
	 * @param i the block group number
	 * @return the block group with number <i>
	 */
	Ext2BlockGrp *get(size_t i) {
		return _groups + i;
	}

	/**
	 * Marks the superblock as dirty
	 */
	void markDirty() {
		_dirty = true;
	}

	/**
	 * Updates the block-group-descriptor-table, if it is dirty
	 */
	void update();

#if DEBUGGING
	/**
	 * Prints the given block-group
	 *
	 * @param no the block-group-number
	 */
	void print(block_t no);

private:
	void printRanges(const char *name,block_t first,block_t max,uint8_t *bitmap);
#endif

private:
	bool _dirty;
	Ext2BlockGrp *_groups;
	Ext2FileSystem *_fs;
};
