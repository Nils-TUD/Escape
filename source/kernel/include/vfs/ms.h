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

#include <esc/pathtree.h>
#include <vfs/dir.h>
#include <common.h>

/**
 * Mountspace representation in the VFS.
 */
class VFSMS : public VFSDir {
public:
	/**
	 * Creates an empty mountspace file.
	 *
	 * @param u the user
	 * @param parent the parent-node
	 * @param id the mountspace id
	 * @param name the node-name
	 * @param mode the mode to set
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSMS(const fs::User &u,VFSNode *parent,uint64_t id,char *name,uint mode,bool &success);

	virtual bool isDeletable() const {
		return false;
	}

	/**
	 * @return the mountspace id
	 */
	uint64_t id() const {
		return _id;
	}

private:
	uint64_t _id;
};
