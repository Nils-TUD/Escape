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

#include <mem/cache.h>
#include <mem/useraccess.h>
#include <sys/messages.h>
#include <task/proc.h>
#include <vfs/channel.h>
#include <vfs/device.h>
#include <vfs/node.h>
#include <vfs/vfs.h>
#include <assert.h>
#include <common.h>
#include <errno.h>
#include <log.h>
#include <spinlock.h>
#include <video.h>

#define PRINT_MSGS			0

SpinLock VFSDevice::msgLock;
uint16_t VFSDevice::nextRid = 1;

/* block- and file-devices are none-empty by default, because their data is always available */
VFSDevice::VFSDevice(const fs::User &u,VFSNode *p,char *n,uint m,uint type,uint ops,bool &success)
		: VFSNode(u,n,buildMode(type) | (m & MODE_PERM),success),
		  owner(Proc::getRunning()), creator(Thread::getRunning()->getTid()),
		  funcs(ops), msgCount(0), lastClient() {
	if(!success)
		return;

	/* auto-destroy on the last close() */
	refCount--;
	append(p);
}

uint VFSDevice::buildMode(uint type) {
	uint mode = 0;
	if(type == DEV_TYPE_BLOCK)
		mode |= MODE_TYPE_BLKDEV;
	else if(type == DEV_TYPE_CHAR)
		mode |= MODE_TYPE_CHARDEV;
	else if(type == DEV_TYPE_FILE)
		mode |= MODE_TYPE_FILEDEV;
	else if(type == DEV_TYPE_FS)
		mode |= MODE_TYPE_FSDEV;
	else {
		assert(type == DEV_TYPE_SERVICE);
		mode |= MODE_TYPE_SERVDEV;
	}
	return mode;
}

ssize_t VFSDevice::getSize() {
	return msgCount;
}

void VFSDevice::close(OpenFile *file,A_UNUSED int msgid) {
	if(file->getFlags() & VFS_DEVICE) {
		/* wakeup all threads that may be waiting for this node so they can check
		 * whether they are affected by the remove of this device and perform the corresponding
		 * action */
		/* do that first because otherwise the client-nodes are already gone :) */
		wakeupClients();
		destroy();
	}
	else
		unref();
}

void VFSDevice::chanRemoved(const VFSChannel *chan) {
	/* we don't have to lock this, because its only called in unref(), which can only
	 * be called when the treelock is held. i.e. it is not possible during getwork() */
	if(lastClient == chan)
		lastClient = chan->next;
	remMsgs(chan->sendList.length());
}

void VFSDevice::bindto(tid_t tid) {
	bool valid;
	const VFSNode *n = openDir(true,&valid);
	creator = tid;
	if(valid) {
		while(n != NULL) {
			VFSChannel *chan = const_cast<VFSChannel*>(static_cast<const VFSChannel*>(n));
			chan->bindto(tid);
			n = n->next;
		}
	}
	closeDir(true);
}

int VFSDevice::getWork(uint flags) {
	Thread *t = Thread::getRunning();
	tid_t tid = t->getTid();

	while(true) {
		{
			LockGuard<SpinLock> g(&msgLock);
			int fd = getClientFd(tid);
			/* if we've found one or we shouldn't block, stop here */
			if(EXPECT_TRUE(fd >= 0 || (flags & GW_NOBLOCK)))
				return fd >= 0 ? fd : -ENOCLIENT;

			/* wait for a client (accept signals) */
			t->wait(EV_CLIENT,(evobj_t)this);
		}

		Thread::switchAway();
		if(EXPECT_FALSE(t->hasSignal()))
			return -EINTR;
	}
	A_UNREACHED;
}

int VFSDevice::getClientFd(tid_t tid) {
	/* search for a slot that needs work */
	bool valid;
	const VFSNode *n = openDir(true,&valid);

	/* if there are no messages at all or the node is invalid, stop right now */
	if(!valid || msgCount == 0) {
		closeDir(true);
		return -ENOCLIENT;
	}

	/* this is a bit more complicated because we want to do it in a fair way. that means every
	 * process that requests something should be served at some time. therefore we store the last
	 * served client and continue from the next one. */
	const VFSNode *first = n;
	const VFSNode *last = lastClient;
	n = last ? last->next : first;

searchBegin:
	while(n != NULL && n != last) {
		/* data available? */
		const VFSChannel *chan = static_cast<const VFSChannel*>(n);
		if(chan->getHandler() == tid && chan->sendList.length() > 0) {
			lastClient = const_cast<VFSNode*>(n);
			closeDir(true);
			return static_cast<const VFSChannel*>(lastClient)->getFd();
		}

		n = n->next;
	}

	if(lastClient) {
		lastClient = NULL;
		last = last->next;
		n = first;
		goto searchBegin;
	}

	closeDir(true);
	return -ENOCLIENT;
}

int VFSDevice::send(VFSChannel *chan,ushort flags,msgid_t id,USER const void *data1,
                    size_t size1,USER const void *data2,size_t size2) {
	esc::SList<VFSChannel::Message> *list;
	VFSChannel::Message *msg1,*msg2 = NULL;
	int res;

	if(EXPECT_FALSE(!isAlive()))
		return -EDESTROYED;

	/* devices write to the receive-list (which will be read by other processes) */
	if(flags & VFS_DEVICE) {
		assert(data2 == NULL && size2 == 0);
		list = &chan->recvList;
	}
	/* other processes write to the send-list (which will be read by the driver) */
	else
		list = &chan->sendList;

	/* create message and copy data to it */
	msg1 = new (size1) VFSChannel::Message(size1);
	if(EXPECT_FALSE(msg1 == NULL))
		return -ENOMEM;

	if(EXPECT_TRUE(data1)) {
		if(EXPECT_FALSE((res = UserAccess::read(msg1 + 1,data1,size1)) < 0))
			goto errorMsg1;
	}

	if(EXPECT_FALSE(data2)) {
		msg2 = new (size2) VFSChannel::Message(size2);
		if(EXPECT_FALSE(msg2 == NULL)) {
			res = -ENOMEM;
			goto errorMsg1;
		}

		if(EXPECT_FALSE((res = UserAccess::read(msg2 + 1,data2,size2)) < 0))
			goto errorMsg2;
	}

	{
		/* note that we do that here, because memcpy can fail because the page is swapped out for
		 * example. we can't hold the lock during that operation */
		LockGuard<SpinLock> g(&msgLock);

		if(~flags & VFS_DEVICE) {
			/* for clients, we generate a new unique request-id */
			id &= 0xFFFF;
			/* prevent to set the MSB. otherwise the return-value would be negative (on 32-bit) */
			id |= ((nextRid++) & 0x7FFF) << 16;
			/* it can't be 0. this is a special value */
			if(id >> 16 == 0)
				id |= 0x00010000;

			/* notify driver */
			addMsgs(1);
			if(EXPECT_FALSE(msg2))
				addMsgs(1);
			Sched::wakeup(EV_CLIENT,(evobj_t)this,true);
		}
		else {
			/* for devices, we just use whatever the driver gave us */

			/* notify receivers */
			Sched::wakeup(EV_RECEIVED_MSG,(evobj_t)chan,true);
		}

		/* append to list */
		msg1->id = id;
		list->append(msg1);
		if(EXPECT_FALSE(msg2)) {
			msg2->id = id;
			list->append(msg2);
		}
	}

#if PRINT_MSGS
	{
		Thread *t = Thread::getRunning();
		Proc *p = Proc::getByPid(pid);
		Log::get().writef("%2d:%2d(%-12.12s) -> %5u:%5u (%4d b) %#x (%s)\n",
				t->getTid(),pid,p ? p->getProgram() : "??",id >> 16,id & 0xFFFF,size1,chan,getPath());
		if(data2) {
			Log::get().writef("%2d:%2d(%-12.12s) -> %5u:%5u (%4d b) %#x (%s)\n",
					t->getTid(),pid,p ? p->getProgram() : "??",id >> 16,id & 0xFFFF,size2,chan,getPath());
		}
	}
#endif
	return id;

errorMsg2:
	delete msg2;
errorMsg1:
	delete msg1;
	return res;
}

ssize_t VFSDevice::receive(VFSChannel *chan,ushort flags,msgid_t *id,USER void *data,size_t size) {
	esc::SList<VFSChannel::Message> *list;
	Thread *t = Thread::getRunning();
	VFSNode *waitNode;
	VFSChannel::Message *msg;
	size_t event;
	ssize_t res;

	/* determine list and event to use */
	if(flags & VFS_DEVICE) {
		event = EV_CLIENT;
		list = &chan->sendList;
		waitNode = this;
	}
	else {
		event = EV_RECEIVED_MSG;
		list = &chan->recvList;
		waitNode = chan;
	}

	/* wait until a message arrives */
	msgLock.down();
	while((msg = getMsg(list,*id,flags)) == NULL) {
		if(EXPECT_FALSE((flags & (VFS_NOBLOCK | VFS_BLOCK)) == VFS_NOBLOCK)) {
			msgLock.up();
			return -EWOULDBLOCK;
		}
		/* if the channel has already been closed, there is no hope of success here */
		if(EXPECT_FALSE(chan->closed || !chan->isAlive())) {
			msgLock.up();
			return -EDESTROYED;
		}
		t->wait(event,(evobj_t)waitNode);
		msgLock.up();

		if(flags & VFS_SIGNALS) {
			Thread::switchAway();
			if(EXPECT_FALSE(t->hasSignal()))
				return -EINTR;
		}
		else
			Thread::switchNoSigs();

		/* if we waked up and there is no message, the driver probably died */
		msgLock.down();
		if(EXPECT_FALSE(!isAlive())) {
			msgLock.up();
			return -EDESTROYED;
		}
	}

	if(event == EV_CLIENT)
		remMsgs(1);
	msgLock.up();

#if PRINT_MSGS
	Proc *p = Proc::getByPid(pid);
	Log::get().writef("%2d:%2d(%-12.12s) <- %5u:%5u (%4d b) %#x (%s)\n",
			t->getTid(),pid,p ? p->getProgram() : "??",msg->id >> 16,msg->id & 0xFFFF,
			msg->length,chan,getPath());
#endif

	if(EXPECT_FALSE(data && msg->length > size)) {
		Log::get().writef("INVALID: len=%zu, size=%zu\n",msg->length,size);
		delete msg;
		return -EINVAL;
	}

	/* copy data and id */
	if(EXPECT_TRUE(data)) {
		if(EXPECT_FALSE((res = UserAccess::write(data,msg + 1,msg->length)) < 0))
			return res;
	}
	if(EXPECT_TRUE(id))
		*id = msg->id;

	res = msg->length;
	delete msg;
	return res;
}

VFSChannel::Message *VFSDevice::getMsg(esc::SList<VFSChannel::Message> *list,msgid_t mid,ushort flags) {
	/* drivers get always the first message */
	if(flags & VFS_DEVICE)
		return list->removeFirst();

	/* find the message for the given id */
	VFSChannel::Message *p = NULL;
	for(auto it = list->begin(); it != list->end(); p = &*it, ++it) {
		/* either it's a "for anybody" message, or the id has to match */
		if(it->id == mid || (it->id >> 16) == 0) {
			list->removeAt(p,&*it);
			return &*it;
		}
	}
	return NULL;
}

void VFSDevice::print(OStream &os) const {
	bool valid;
	const VFSNode *chan = openDir(false,&valid);
	if(valid) {
		os.writef("%s (creator=%d, lastClient=%s):\n",name,creator,lastClient ? lastClient->getName() : "-");
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

void VFSDevice::wakeupClients() {
	bool valid;
	const VFSNode *n = openDir(true,&valid);
	if(valid) {
		while(n != NULL) {
			Sched::wakeup(EV_RECEIVED_MSG,(evobj_t)n);
			n = n->next;
		}
	}
	closeDir(true);
}
