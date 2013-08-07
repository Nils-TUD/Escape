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
#include <sys/task/event.h>
#include <sys/task/proc.h>
#include <sys/mem/cache.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/device.h>
#include <sys/video.h>
#include <sys/spinlock.h>
#include <esc/messages.h>
#include <assert.h>
#include <errno.h>

#define DRV_IMPL(funcs,func)		(((funcs) & (func)) != 0)

/* block- and file-devices are none-empty by default, because their data is always available */
VFSDevice::VFSDevice(pid_t pid,VFSNode *p,char *n,uint type,uint ops,bool &success)
		: VFSNode(pid,n,buildMode(type),success),
		  isEmpty(type != DEV_TYPE_BLOCK && type != DEV_TYPE_FILE), funcs(ops), msgCount(0), lastClient() {
	if(!success)
		return;

	/* auto-destroy on the last close() */
	refCount--;
	append(p);
}

uint VFSDevice::buildMode(uint type) {
	uint mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	if(type == DEV_TYPE_BLOCK)
		mode |= MODE_TYPE_BLKDEV;
	else if(type == DEV_TYPE_CHAR)
		mode |= MODE_TYPE_CHARDEV;
	else if(type == DEV_TYPE_FILE)
		mode |= MODE_TYPE_FILEDEV;
	else if(type == DEV_TYPE_SERVICE)
		mode |= MODE_TYPE_SERVDEV;
	else {
		assert(type == DEV_TYPE_FS);
		mode |= MODE_TYPE_FSDEV;
	}
	return mode;
}

size_t VFSDevice::getSize(A_UNUSED pid_t pid) const {
	return msgCount;
}

void VFSDevice::close(A_UNUSED pid_t pid,A_UNUSED OpenFile *file) {
	/* wakeup all threads that may be waiting for this node so they can check
	 * whether they are affected by the remove of this device and perform the corresponding
	 * action */
	/* do that first because otherwise the client-nodes are already gone :) */
	wakeupClients(EV_RECEIVED_MSG | EV_DATA_READABLE,true);
	destroy();
}

int VFSDevice::setReadable(bool readable) {
	if(!supports(DEV_READ))
		return -ENOTSUP;
	bool wasEmpty = isEmpty;
	isEmpty = !readable;
	if(wasEmpty && readable)
		wakeupClients(EV_RECEIVED_MSG | EV_DATA_READABLE,true);
	return 0;
}

VFSNode *VFSDevice::getWork(bool *cont,bool *retry) {
	const VFSNode *n,*last;
	bool valid;
	/* this is a bit more complicated because we want to do it in a fair way. that means every
	 * process that requests something should be served at some time. therefore we store the last
	 * served client and continue from the next one. */

	/* search for a slot that needs work */
	/* we don't need to lock the device-data here; the node with openDir() is sufficient */
	/* because it can't be called twice because the waitLock in vfs prevents it. and nothing of the
	 * device-data that is used here can be changed during this procedure. */
	n = openDir(true,&valid);
	/* if there are no messages at all or the node is invalid, stop right now */
	if(!valid || msgCount == 0) {
		closeDir(true);
		return NULL;
	}

	last = lastClient;
	if(last != NULL) {
		if(last->next == NULL) {
			closeDir(true);
			/* if we have checked all clients in this device, give the other devices
			 * a chance (if there are any others) */
			lastClient = NULL;
			*retry = true;
			return NULL;
		}
		else
			n = last->next;
	}

searchBegin:
	while(n != NULL && n != last) {
		/* data available? */
		if(static_cast<const VFSChannel*>(n)->hasWork()) {
			n->increaseRefs();
			closeDir(true);
			*cont = false;
			lastClient = n;
			return const_cast<VFSNode*>(n);
		}
		n = n->next;
	}

	/* if we have looked through all nodes and the last one has a message again, we have to
	 * store it because we'll not check it again */
	if(last && n == last && static_cast<const VFSChannel*>(n)->hasWork()) {
		n->increaseRefs();
		closeDir(true);
		return const_cast<VFSNode*>(n);
	}
	closeDir(true);

	if(lastClient) {
		n = openDir(true,&valid);
		if(!valid) {
			closeDir(true);
			return NULL;
		}
		lastClient = NULL;
		goto searchBegin;
	}
	return NULL;
}

void VFSDevice::print(OStream &os) const {
	bool valid;
	const VFSNode *chan = openDir(false,&valid);
	if(valid) {
		os.writef("%s (%s):\n",name,isEmpty ? "empty" : "full");
		while(chan != NULL) {
			os.pushIndent();
			chan->print(os);
			os.popIndent();
			chan = chan->next;
		}
		os.writef("\n");
	}
	closeDir(false);
}

void VFSDevice::wakeupClients(uint events,bool locked) {
	bool valid;
	const VFSNode *n = openDir(locked,&valid);
	if(valid) {
		while(n != NULL) {
			Event::wakeupm(events,(evobj_t)n);
			n = n->next;
		}
	}
	closeDir(locked);
}
