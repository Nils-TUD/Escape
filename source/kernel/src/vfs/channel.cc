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
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/sllnodes.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/device.h>
#include <sys/vfs/openfile.h>
#include <sys/video.h>
#include <sys/spinlock.h>
#include <sys/log.h>
#include <esc/messages.h>
#include <esc/sllist.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define PRINT_MSGS			0

extern klock_t waitLock;

VFSChannel::VFSChannel(pid_t pid,VFSNode *p,bool &success)
		: VFSNode(pid,generateId(pid),MODE_TYPE_CHANNEL | S_IRUSR | S_IWUSR,success), used(false),
		  closed(false), curClient(), sendList(), recvList() {
	if(!success)
		return;

	/* auto-destroy on the last close() */
	refCount--;
	append(p);
}

void VFSChannel::invalidate() {
	/* we have to reset the last client for the device here */
	if(parent)
		static_cast<VFSDevice*>(parent)->clientRemoved(this);
	recvList.deleteAll();
	sendList.deleteAll();
}

int VFSChannel::isSupported(int op) const {
	VFSNode::acquireTree();
	if(!isAlive()) {
		VFSNode::releaseTree();
		return -EDESTROYED;
	}
	/* if the driver doesn't implement read, its an error */
	if(!static_cast<VFSDevice*>(parent)->supports(op)) {
		VFSNode::releaseTree();
		return -ENOTSUP;
	}
	VFSNode::releaseTree();
	return 0;
}

ssize_t VFSChannel::open(pid_t pid,OpenFile *file,uint flags) {
	ssize_t res;
	sArgsMsg msg;
	msgid_t mid;
	Thread *t = Thread::getRunning();

	if((res = isSupported(DEV_OPEN)) < 0)
		return res == -ENOTSUP ? 0 : res;

	/* send msg to driver */
	msg.arg1 = flags;
	res = file->sendMsg(pid,MSG_DEV_OPEN,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		return res;

	/* receive response */
	t->addResource();
	do {
		res = file->receiveMsg(pid,&mid,&msg,sizeof(msg),true);
		vassert(res < 0 || mid == MSG_DEV_OPEN_RESP,"mid=%u, res=%zd, node=%s:%p",
		        mid,res,getPath(),this);
	}
	while(res == -EINTR);
	t->remResource();
	if(res < 0)
		return res;
	return msg.arg1;
}

void VFSChannel::close(pid_t pid,OpenFile *file) {
	if(!isAlive())
		unref();
	else {
		if(file->isDevice()) {
			used = false;
			curClient = NULL;
		}

		/* TODO actually, this doesn't work (we need a lock for refCount) */
		if(refCount > 1) {
			if(refCount == 2 && closed)
				unref();
			unref();
		}
		else {
			/* if the driver implemented close, notify him */
			if(static_cast<VFSDevice*>(parent)->supports(DEV_CLOSE))
				file->sendMsg(pid,MSG_DEV_CLOSE,NULL,0,NULL,0);

			/* if there are message for the driver we don't want to throw them away */
			/* if there are any in the receivelist (and no references of the node) we
			 * can throw them away because no one will read them anymore
			 * (it means that the client has already closed the file) */
			/* note also that we can assume that the driver is still running since we
			 * would have deleted the whole device-node otherwise */
			if(sendList.length() == 0)
				unref();
			else
				closed = true;
		}
	}
}

off_t VFSChannel::seek(A_UNUSED pid_t pid,off_t position,off_t offset,uint whence) {
	switch(whence) {
		case SEEK_SET:
			return offset;
		case SEEK_CUR:
			return position + offset;
		default:
		case SEEK_END:
			/* not supported for devices */
			return -ESPIPE;
	}
}

size_t VFSChannel::getSize(A_UNUSED pid_t pid) const {
	return sendList.length() + recvList.length();
}

ssize_t VFSChannel::read(pid_t pid,OpenFile *file,USER void *buffer,off_t offset,size_t count) {
	ssize_t res;
	msgid_t mid;
	sArgsMsg msg;
	Event::WaitObject obj;
	Thread *t = Thread::getRunning();

	if((res = isSupported(DEV_READ)) < 0)
		return res;

	/* wait until there is data available, if necessary */
	obj.events = EV_DATA_READABLE;
	obj.object = (evobj_t)file;
	res = VFS::waitFor(&obj,1,0,file->shouldBlock(),KERNEL_PID,0);
	if(res < 0)
		return res;

	/* send msg to driver */
	msg.arg1 = offset;
	msg.arg2 = count;
	res = file->sendMsg(pid,MSG_DEV_READ,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		return res;

	/* read response and ensure that we don't get killed until we've received both messages
	 * (otherwise the channel might get in an inconsistent state) */
	t->addResource();
	do {
		res = file->receiveMsg(pid,&mid,&msg,sizeof(msg),true);
		vassert(res < 0 || mid == MSG_DEV_READ_RESP,"mid=%u, res=%zd, node=%s:%p",
				mid,res,getPath(),this);
	}
	while(res == -EINTR);
	t->remResource();
	if(res < 0)
		return res;

	/* handle response */
	if(msg.arg2 != READABLE_DONT_SET) {
		VFSNode::acquireTree();
		if(parent)
			static_cast<VFSDevice*>(parent)->setReadable(msg.arg2);
		VFSNode::releaseTree();
	}
	if((long)msg.arg1 <= 0)
		return msg.arg1;

	/* read data */
	t->addResource();
	do
		res = file->receiveMsg(pid,NULL,buffer,count,true);
	while(res == -EINTR);
	t->remResource();
	return res;
}

ssize_t VFSChannel::write(pid_t pid,OpenFile *file,USER const void *buffer,off_t offset,size_t count) {
	msgid_t mid;
	ssize_t res;
	sArgsMsg msg;
	Thread *t = Thread::getRunning();

	if((res = isSupported(DEV_WRITE)) < 0)
		return res;

	/* send msg and data to driver */
	msg.arg1 = offset;
	msg.arg2 = count;
	res = file->sendMsg(pid,MSG_DEV_WRITE,&msg,sizeof(msg),buffer,count);
	if(res < 0)
		return res;

	/* read response */
	t->addResource();
	do {
		res = file->receiveMsg(pid,&mid,&msg,sizeof(msg),true);
		vassert(res < 0 || mid == MSG_DEV_WRITE_RESP,"mid=%u, res=%zd, node=%s:%p",
				mid,res,getPath(),this);
	}
	while(res == -EINTR);
	t->remResource();
	if(res < 0)
		return res;
	return msg.arg1;
}

ssize_t VFSChannel::send(A_UNUSED pid_t pid,ushort flags,msgid_t id,USER const void *data1,
                         size_t size1,USER const void *data2,size_t size2) {
	SList<Message> *list;
	Thread *t = Thread::getRunning();
	Message *msg1,*msg2 = NULL;
	Thread *recipient = t;

#if PRINT_MSGS
	Proc *p = Proc::getByPid(pid);
	Log::get().writef("%2d:%2d(%-12.12s) -> %6d (%4d b) %#x (%s)\n",
			t->getTid(),pid,p ? p->getCommand() : "??",id,size1,this,getPath());
	if(data2) {
		Log::get().writef("%2d:%2d(%-12.12s) -> %6d (%4d b) %#x (%s)\n",
				t->getTid(),pid,p ? p->getCommand() : "??",id,size2,this,getPath());
	}
#endif

	/* devices write to the receive-list (which will be read by other processes) */
	if(flags & VFS_DEVICE) {
		assert(data2 == NULL && size2 == 0);
		list = &recvList;
	}
	/* other processes write to the send-list (which will be read by the driver) */
	else
		list = &sendList;

	/* create message and copy data to it */
	msg1 = (Message*)Cache::alloc(sizeof(Message) + size1);
	if(msg1 == NULL)
		return -ENOMEM;

	msg1->length = size1;
	msg1->id = id;
	if(data1) {
		Thread::addHeapAlloc(msg1);
		memcpy(msg1 + 1,data1,size1);
		Thread::remHeapAlloc(msg1);
	}

	if(data2) {
		msg2 = (Message*)Cache::alloc(sizeof(Message) + size2);
		if(msg2 == NULL)
			return -ENOMEM;

		msg2->length = size2;
		msg2->id = id;
		Thread::addHeapAlloc(msg1);
		Thread::addHeapAlloc(msg2);
		memcpy(msg2 + 1,data2,size2);
		Thread::remHeapAlloc(msg2);
		Thread::remHeapAlloc(msg1);
	}

	/* note that we do that here, because memcpy can fail because the page is swapped out for
	 * example. we can't hold the lock during that operation */
	SpinLock::acquire(&waitLock);

	/* set recipient */
	if(flags & VFS_DEVICE)
		recipient = id >= MSG_DEF_RESPONSE ? curClient : NULL;
	msg1->thread = recipient;
	if(data2)
		msg2->thread = recipient;

	/* append to list */
	list->append(msg1);
	if(msg2)
		list->append(msg2);

	/* notify the driver */
	if(list == &sendList) {
		VFSNode::acquireTree();
		if(parent) {
			static_cast<VFSDevice*>(parent)->addMsg();
			if(msg2)
				static_cast<VFSDevice*>(parent)->addMsg();
			Event::wakeup(EVI_CLIENT,(evobj_t)parent);
		}
		VFSNode::releaseTree();
	}
	else {
		/* notify other possible waiters */
		if(recipient)
			Event::unblock(recipient);
		else
			Event::wakeup(EVI_RECEIVED_MSG,(evobj_t)this);
	}
	SpinLock::release(&waitLock);
	return 0;
}

ssize_t VFSChannel::receive(A_UNUSED pid_t pid,ushort flags,USER msgid_t *id,USER void *data,
                            size_t size,bool block,bool ignoreSigs) {
	SList<Message> *list;
	Thread *t = Thread::getRunning();
	VFSNode *waitNode;
	Message *msg;
	size_t event;
	ssize_t res;

	/* determine list and event to use */
	if(flags & VFS_DEVICE) {
		event = EVI_CLIENT;
		list = &sendList;
		waitNode = parent;
	}
	else {
		event = EVI_RECEIVED_MSG;
		list = &recvList;
		waitNode = this;
	}

	/* wait until a message arrives */
	SpinLock::acquire(&waitLock);
	while((msg = getMsg(t,list,flags)) == NULL) {
		if(!block) {
			SpinLock::release(&waitLock);
			return -EWOULDBLOCK;
		}
		/* if the channel has already been closed, there is no hope of success here */
		if(closed) {
			SpinLock::release(&waitLock);
			return -EDESTROYED;
		}
		Event::wait(t,event,(evobj_t)waitNode);
		SpinLock::release(&waitLock);

		if(ignoreSigs)
			Thread::switchNoSigs();
		else
			Thread::switchAway();

		if(Signals::hasSignalFor(t->getTid()))
			return -EINTR;
		/* if we waked up and there is no message, the driver probably died */
		if(!isAlive())
			return -EDESTROYED;
		SpinLock::acquire(&waitLock);
	}

	if(event == EVI_CLIENT) {
		VFSNode::acquireTree();
		if(parent)
			static_cast<VFSDevice*>(parent)->remMsg();
		VFSNode::releaseTree();
		curClient = msg->thread;
	}
	SpinLock::release(&waitLock);
	if(data && msg->length > size) {
		Cache::free(msg);
		return -EINVAL;
	}

#if PRINT_MSGS
	Proc *p = Proc::getByPid(pid);
	Log::get().writef("%2d:%2d(%-12.12s) <- %6d (%4d b) %#x (%s)\n",
			t->getTid(),pid,p ? p->getCommand() : "??",msg->id,msg->length,this,getPath());
#endif

	/* copy data and id; since it may fail we have to ensure that our resources are free'd */
	Thread::addHeapAlloc(msg);
	if(data)
		memcpy(data,msg + 1,msg->length);
	if(id)
		*id = msg->id;
	Thread::remHeapAlloc(msg);

	res = msg->length;
	Cache::free(msg);
	return res;
}

VFSChannel::Message *VFSChannel::getMsg(Thread *t,SList<Message> *list,ushort flags) {
	/* drivers get always the first message */
	if(flags & VFS_DEVICE)
		return list->removeFirst();

	/* find the message for this client-thread */
	Message *p = NULL;
	for(auto it = list->begin(); it != list->end(); p = &*it, ++it) {
		if(it->thread == NULL || it->thread == t) {
			list->removeAt(p,&*it);
			return &*it;
		}
	}
	return NULL;
}

void VFSChannel::print(OStream &os) const {
	const SList<Message> *lists[] = {&sendList,&recvList};
	for(size_t i = 0; i < ARRAY_SIZE(lists); i++) {
		size_t count = lists[i]->length();
		os.writef("Channel %s %s: (%zu,%s)\n",name,i ? "recvs" : "sends",count,used ? "used" : "-");
		for(auto it = lists[i]->cbegin(); it != lists[i]->cend(); ++it) {
			os.writef("\tid=%u len=%zu, thread=%d:%d:%s\n",it->id,it->length,
					it->thread ? it->thread->getTid() : -1,
					it->thread ? it->thread->getProc()->getPid() : -1,
					it->thread ? it->thread->getProc()->getCommand() : "");
		}
	}
}
