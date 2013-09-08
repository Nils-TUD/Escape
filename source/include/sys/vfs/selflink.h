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

#pragma once

#include <sys/common.h>
#include <sys/vfs/link.h>
#include <errno.h>

class VFSSelfLink : public VFSLink {
public:
	/**
	 * Creates a link to /system/processes/<current> with name <name> in <parent>.
	 *
	 * @param pid the process-id
	 * @param parent the parent-node
	 * @param name the name
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSSelfLink(pid_t pid,VFSNode *parent,char *name,bool &success)
			: VFSLink(pid,parent,name,S_IFLNK | (DIR_DEF_MODE & MODE_PERM),success) {
	}

	virtual const VFSNode *resolve() const;
};
