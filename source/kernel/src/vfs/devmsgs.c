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
#include <sys/vfs/devmsgs.h>
#include <sys/vfs/device.h>
#include <sys/vfs/channel.h>
#include <sys/mem/cache.h>
#include <sys/mem/paging.h>
#include <sys/mem/vmm.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
#include <sys/task/signals.h>
#include <esc/sllist.h>
#include <esc/messages.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

ssize_t vfs_devmsgs_open(pid_t pid,file_t file,sVFSNode *node,uint flags) {
	ssize_t res;
	sArgsMsg msg;
	msgid_t mid;
	sThread *t = thread_getRunning();

	if(node->name == NULL)
		return -EDESTROYED;
	/* if the driver doesn't implement open, its ok */
	if(!vfs_device_supports(node->parent,DEV_OPEN))
		return 0;

	/* send msg to driver */
	msg.arg1 = flags;
	res = vfs_sendMsg(pid,file,MSG_DEV_OPEN,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		return res;

	/* receive response */
	t->resources++;
	do {
		res = vfs_receiveMsg(pid,file,&mid,&msg,sizeof(msg),true);
		vassert(res < 0 || mid == MSG_DEV_OPEN_RESP,"mid=%u, res=%zd, node=%s:%p",
				mid,res,vfs_node_getPath(vfs_node_getNo(node)),node);
	}
	while(res == -EINTR);
	t->resources--;
	if(res < 0)
		return res;
	return msg.arg1;
}

ssize_t vfs_devmsgs_read(pid_t pid,file_t file,sVFSNode *node,USER void *buffer,off_t offset,
		size_t count) {
	ssize_t res;
	msgid_t mid;
	sArgsMsg msg;
	sWaitObject obj;
	sThread *t = thread_getRunning();

	if(node->name == NULL)
		return -EDESTROYED;
	/* if the driver doesn't implement open, its an error */
	if(!vfs_device_supports(node->parent,DEV_READ))
		return -ENOTSUP;

	/* wait until there is data available, if necessary */
	obj.events = EV_DATA_READABLE;
	obj.object = file;
	res = vfs_waitFor(&obj,1,0,vfs_shouldBlock(file),KERNEL_PID,0);
	if(res < 0)
		return res;

	/* send msg to driver */
	msg.arg1 = offset;
	msg.arg2 = count;
	res = vfs_sendMsg(pid,file,MSG_DEV_READ,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		return res;

	/* read response and ensure that we don't get killed until we've received both messages
	 * (otherwise the channel might get in an inconsistent state) */
	t->resources++;
	do {
		res = vfs_receiveMsg(pid,file,&mid,&msg,sizeof(msg),true);
		vassert(res < 0 || mid == MSG_DEV_READ_RESP,"mid=%u, res=%zd, node=%s:%p",
				mid,res,vfs_node_getPath(vfs_node_getNo(node)),node);
	}
	while(res == -EINTR);
	t->resources--;
	if(res < 0)
		return res;

	/* handle response */
	vfs_device_setReadable(node->parent,msg.arg2);
	if((long)msg.arg1 <= 0)
		return msg.arg1;

	/* read data */
	t->resources++;
	do
		res = vfs_receiveMsg(pid,file,NULL,buffer,count,true);
	while(res == -EINTR);
	t->resources--;
	return res;
}

ssize_t vfs_devmsgs_write(pid_t pid,file_t file,sVFSNode *node,USER const void *buffer,off_t offset,
		size_t count) {
	msgid_t mid;
	ssize_t res;
	sArgsMsg msg;
	sThread *t = thread_getRunning();

	if(node->name == NULL)
		return -EDESTROYED;
	/* if the driver doesn't implement open, its an error */
	if(!vfs_device_supports(node->parent,DEV_WRITE))
		return -ENOTSUP;

	/* send msg and data to driver */
	msg.arg1 = offset;
	msg.arg2 = count;
	res = vfs_sendMsg(pid,file,MSG_DEV_WRITE,&msg,sizeof(msg),buffer,count);
	if(res < 0)
		return res;

	/* read response */
	t->resources++;
	do {
		res = vfs_receiveMsg(pid,file,&mid,&msg,sizeof(msg),true);
		vassert(res < 0 || mid == MSG_DEV_WRITE_RESP,"mid=%u, res=%zd, node=%s:%p",
				mid,res,vfs_node_getPath(vfs_node_getNo(node)),node);
	}
	while(res == -EINTR);
	t->resources--;
	if(res < 0)
		return res;
	return msg.arg1;
}

void vfs_devmsgs_close(pid_t pid,file_t file,sVFSNode *node) {
	/* if the driver doesn't implement close, stop here */
	if(node->name == NULL || !vfs_device_supports(node->parent,DEV_CLOSE))
		return;

	vfs_sendMsg(pid,file,MSG_DEV_CLOSE,NULL,0,NULL,0);
}
