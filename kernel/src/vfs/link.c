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

#include <sys/common.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/link.h>
#include <sys/vfs/node.h>

sVFSNode *vfs_link_create(tPid pid,sVFSNode *node,char *name,sVFSNode *target) {
	sVFSNode *child = vfs_node_create(node,name);
	if(child == NULL)
		return NULL;
	child->readHandler = NULL;
	child->writeHandler = NULL;
	child->seek = NULL;
	child->destroy = NULL;
	child->owner = pid;
	child->mode = MODE_TYPE_LINK | MODE_OWNER_READ | MODE_OTHER_READ;
	child->data = target;
	return child;
}

sVFSNode *vfs_link_resolve(sVFSNode *node) {
	return (sVFSNode*)node->data;
}
