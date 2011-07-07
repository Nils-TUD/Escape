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

#include <sys/common.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/link.h>
#include <sys/vfs/node.h>
#include <sys/task/proc.h>

sVFSNode *vfs_link_create(pid_t pid,sVFSNode *parent,char *name,const sVFSNode *target) {
	sVFSNode *child = vfs_node_create(pid,parent,name);
	if(child == NULL)
		return NULL;
	child->read = NULL;
	child->write = NULL;
	child->seek = NULL;
	child->destroy = NULL;
	child->close = NULL;
	child->mode = S_IFLNK | (target->mode & MODE_PERM);
	child->data = (void*)target;
	return child;
}

sVFSNode *vfs_link_resolve(const sVFSNode *node) {
	return (sVFSNode*)node->data;
}
