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
#include <sys/vfs/vfs.h>

/**
 * Creates a new link with name <name> in <parent> that links to <target>
 *
 * @param pid the process-id
 * @param parent the parent-node
 * @param name the name
 * @param target the target-node
 * @return the created node or NULL
 */
sVFSNode *vfs_link_create(pid_t pid,sVFSNode *parent,char *name,const sVFSNode *target);

/**
 * @param node the link-node
 * @return the link-target
 */
sVFSNode *vfs_link_resolve(const sVFSNode *node);
