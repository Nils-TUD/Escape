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

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include <sys/common.h>
#include <sys/vfs/vfs.h>

sVFSNode *vfs_chan_create(tPid pid,sVFSNode *parent);
bool vfs_chan_hasReply(sVFSNode *node);
bool vfs_chan_hasWork(sVFSNode *node);
s32 vfs_chan_send(tPid pid,tFileNo file,sVFSNode *n,tMsgId id,const u8 *data,u32 size);
s32 vfs_chan_receive(tPid pid,tFileNo file,sVFSNode *node,tMsgId *id,u8 *data,u32 size);


#if DEBUGGING

void vfs_chan_dbg_print(sVFSNode *n);

#endif

#endif /* CHANNEL_H_ */
