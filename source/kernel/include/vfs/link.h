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

#include <vfs/node.h>
#include <common.h>
#include <errno.h>

class VFSLink : public VFSNode {
protected:
	explicit VFSLink(pid_t pid,VFSNode *parent,char *name,uint mode,bool &success)
			: VFSNode(pid,name,mode,success), target(NULL) {
		append(parent);
	}

public:
	/**
	 * Creates a new link with name <name> in <parent> that links to <target>
	 *
	 * @param pid the process-id
	 * @param parent the parent-node
	 * @param name the name
	 * @param target the target-node
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSLink(pid_t pid,VFSNode *parent,char *name,const VFSNode *target,bool &success)
			: VFSNode(pid,name,MODE_TYPE_HARDLINK | (target->getMode() & MODE_PERM),success), target(target) {
		append(parent);
	}

	/**
	 * Sets the new target node
	 *
	 * @param n the node
	 */
	void setTarget(const VFSNode *n) {
		target = n;
	}

	/**
	 * @return the link-target
	 */
	virtual const VFSNode *resolve() const {
		return target;
	}

private:
	const VFSNode *target;
};
