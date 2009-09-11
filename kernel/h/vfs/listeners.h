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

#ifndef LISTENERS_H_
#define LISTENERS_H_

#include <common.h>
#include <vfs/vfs.h>

/* the events we can listen for */
#define VFS_EV_CREATE		1
#define VFS_EV_MODIFY		2
#define VFS_EV_DELETE		4

typedef void (*fVFSListener)(sVFSNode *node,u32 event);

/**
 * Adds a listener for given node and events that will be notified when new childs are created
 *
 * @param node the directory-node
 * @param events the events to get notified about
 * @param listener the function to call
 * @return 0 on success
 */
s32 vfsl_add(sVFSNode *node,u32 events,fVFSListener listener);

/**
 * Removes the given listener for given node that are announced for the given event.
 * Note that this may be more than one listener!
 *
 * @param node the directory-node
 * @param events the events to remove from
 * @param listener the function to call
 * @return 0 on success
 */
void vfsl_remove(sVFSNode *node,u32 events,fVFSListener listener);

/**
 * Notifies all listeners for the given node about <event>
 *
 * @param node the node
 * @param event the event
 */
void vfsl_notify(sVFSNode *node,u32 event);

#endif /* LISTENERS_H_ */
