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
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/filedesc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/device.h>
#include <sys/vfs/openfile.h>
#include <sys/video.h>
#include <sys/spinlock.h>
#include <sys/log.h>
#include <esc/messages.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define PRINT_MSGS			0

extern klock_t waitLock;

VFSChannel::VFSChannel(pid_t pid,VFSNode *p,bool &success)
		: VFSNode(pid,generateId(pid),MODE_TYPE_CHANNEL | S_IXUSR | S_IRUSR | S_IWUSR,success), fd(-1),
		  used(false), closed(false), shmem(NULL), shmemSize(0), curClient(), sendList(), recvList() {
	if(!success)
		return;

	/* auto-destroy on the last close() */
	refCount--;
	append(p);
}

void VFSChannel::invalidate() {
	/* notify potentially waiting clients */
	Sched::wakeup(EV_RECEIVED_MSG,(evobj_t)this);
	// luckily, we have the treelock here which is also used for all other calls to addMsgs/remMsgs.
	// but we get the number of messages in VFS::waitFor() without the treelock. but in this case it
	// doesn't really hurt to remove messages, because we would just give up waiting once, probably
	// call getwork afterwards which will see that there is actually no new message anymore.
	// (we can't acquire the waitlock because we use these locks in the opposite order below)
	// note also that we only get here if there are no references to this node anymore. so it's safe
	// to access the lists.
	if(getParent())
		static_cast<VFSDevice*>(getParent())->remMsgs(sendList.length());
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

void VFSChannel::discardMsgs() {
	SpinLock::acquire(&waitLock);
	// remove from parent
	VFSNode::acquireTree();
	if(getParent())
		static_cast<VFSDevice*>(getParent())->remMsgs(sendList.length());
	VFSNode::releaseTree();

	// now clear lists
	sendList.deleteAll();
	recvList.deleteAll();
	SpinLock::release(&waitLock);
}

ssize_t VFSChannel::open(pid_t pid,const char *path,uint flags,int msgid) {
	ssize_t res = -ENOENT;
	sMsg msg;
	OpenFile *clifile;
	Proc *p;
	bool haveTree = true;
	Thread *t = Thread::getRunning();
	size_t pathLen = 0;

	if(path && (pathLen = strlen(path)) > MAX_MSGSTR_LEN)
		return -ENAMETOOLONG;

	/* give the driver a file-descriptor for this new client; note that we have to do that
	 * immediatly because in close() we assume that the device has already one reference to it */
	VFSNode::acquireTree();
	VFSNode *par = getParent();
	if(!par)
		goto errOpen;
	res = VFS::openFile(par->getOwner(),VFS_MSGS | VFS_DEVICE,this,getNo(),VFS_DEV_NO,&clifile);
	if(res < 0)
		goto errOpen;
	/* getByPid() is ok because if the parent-node exists, the owner does always exist, too */
	p = Proc::getByPid(par->getOwner());
	fd = FileDesc::assoc(p,clifile);
	if(fd < 0) {
		res = fd;
		goto errAssoc;
	}
	VFSNode::releaseTree();
	haveTree = false;

	/* do we need to send an open to the driver? */
	res = isSupported(DEV_OPEN);
	if(res == -ENOTSUP)
		return 0;
	if(res < 0)
		goto errAssoc;

	/* send msg to driver */
	msg.str.arg1 = flags;
	p = Proc::getByPid(pid);
	if(p) {
		msg.str.arg2 = p->getEUid();
		msg.str.arg3 = p->getEGid();
		msg.str.arg4 = p->getPid();
	}
	if(path)
		memcpy(msg.str.s1,path,pathLen + 1);
	res = send(pid,0,msgid,&msg,sizeof(msg.str),NULL,0);
	if(res < 0)
		goto errSend;

	/* receive response */
	t->addResource();
	do
		res = receive(pid,0,NULL,&msg,sizeof(msg),true,false);
	while(res == -EINTR);
	t->remResource();
	if(res < 0)
		goto errSend;
	if((long)msg.args.arg1 < 0) {
		res = msg.args.arg1;
		goto errSend;
	}
	return msg.args.arg1;

errSend:
	FileDesc::unassoc(p,fd);
errAssoc:
	clifile->close(par->getOwner());
errOpen:
	if(haveTree)
		VFSNode::releaseTree();
	return res;
}

void VFSChannel::close(pid_t pid,OpenFile *file,int msgid) {
	if(!isAlive())
		unref();
	else {
		if(file->isDevice())
			destroy();
		/* if there is only the device left, do the real close */
		else if(unref() == 1) {
			send(pid,0,msgid,NULL,0,NULL,0);
			closed = true;
		}
	}
}

off_t VFSChannel::seek(A_UNUSED pid_t pid,off_t position,off_t offset,uint whence) const {
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

int VFSChannel::stat(pid_t pid,USER sFileInfo *info) {
	/* send msg to fs */
	ssize_t res = send(pid,0,MSG_FS_ISTAT,NULL,0,NULL,0);
	if(res < 0)
		return res;

	/* receive response */
	sDataMsg msg;
	do
		res = receive(pid,0,NULL,&msg,sizeof(msg),true,false);
	while(res == -EINTR);
	if(res < 0)
		return res;
	res = msg.arg1;

	/* copy file-info */
	if(res == 0)
		memcpy(info,msg.d,sizeof(sFileInfo));
	return res;
}

static bool useSharedMem(const void *shmem,size_t shmsize,const void *buffer,size_t bufsize) {
	return shmem && (uintptr_t)buffer >= (uintptr_t)shmem &&
		(uintptr_t)buffer + bufsize > (uintptr_t)buffer &&
		(uintptr_t)buffer + bufsize <= (uintptr_t)shmem + shmsize;
}

ssize_t VFSChannel::read(pid_t pid,OpenFile *file,USER void *buffer,off_t offset,size_t count) {
	ssize_t res;
	sArgsMsg msg;
	Thread *t = Thread::getRunning();
	bool useshm = useSharedMem(shmem,shmemSize,buffer,count);

	if((res = isSupported(DEV_READ)) < 0)
		return res;

	/* first do a quick check. waitFor is only necessary to make sure that we don't miss a message. */
	if(!static_cast<VFSDevice*>(getParent())->isReadable()) {
		/* wait until there is data available, if necessary */
		res = VFS::waitFor(EV_DATA_READABLE,(evobj_t)file,0,file->shouldBlock());
		if(res < 0)
			return res;
	}

	/* send msg to driver */
	msg.arg1 = offset;
	msg.arg2 = count;
	msg.arg3 = useshm ? (uintptr_t)buffer - (uintptr_t)shmem : -1;
	res = file->sendMsg(pid,MSG_DEV_READ,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		return res;

	/* read response and ensure that we don't get killed until we've received both messages
	 * (otherwise the channel might get in an inconsistent state) */
	t->addResource();
	do {
		msgid_t mid;
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

	res = msg.arg1;
	if(!useshm) {
		/* read data */
		t->addResource();
		do
			res = file->receiveMsg(pid,NULL,buffer,count,true);
		while(res == -EINTR);
		t->remResource();
	}
	return res;
}

ssize_t VFSChannel::write(pid_t pid,OpenFile *file,USER const void *buffer,off_t offset,size_t count) {
	ssize_t res;
	sArgsMsg msg;
	Thread *t = Thread::getRunning();
	bool useshm = useSharedMem(shmem,shmemSize,buffer,count);

	if((res = isSupported(DEV_WRITE)) < 0)
		return res;

	/* send msg and data to driver */
	msg.arg1 = offset;
	msg.arg2 = count;
	msg.arg3 = useshm ? (uintptr_t)buffer - (uintptr_t)shmem : -1;
	res = file->sendMsg(pid,MSG_DEV_WRITE,&msg,sizeof(msg),useshm ? NULL : buffer,count);
	if(res < 0)
		return res;

	/* read response */
	t->addResource();
	do {
		msgid_t mid;
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

int VFSChannel::sharefile(pid_t pid,OpenFile *file,const char *path,void *cliaddr,size_t size) {
	ssize_t res;
	sStrMsg msg;
	Thread *t = Thread::getRunning();

	if(shmem != NULL)
		return -EEXIST;
	if((res = isSupported(DEV_SHFILE)) < 0)
		return res;

	/* send msg to driver */
	msg.arg1 = size;
	strnzcpy(msg.s1,path,sizeof(msg.s1));
	res = file->sendMsg(pid,MSG_DEV_SHFILE,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		return res;

	/* read response */
	t->addResource();
	do {
		msgid_t mid;
		res = file->receiveMsg(pid,&mid,&msg,sizeof(msg),true);
		vassert(res < 0 || mid == MSG_DEV_SHFILE_RESP,"mid=%u, res=%zd, node=%s:%p",
				mid,res,getPath(),this);
	}
	while(res == -EINTR);
	t->remResource();
	if(res < 0)
		return res;
	if((int)msg.arg1 < 0)
		return msg.arg1;
	shmem = cliaddr;
	shmemSize = size;
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
			t->getTid(),pid,p ? p->getProgram() : "??",id,size1,this,getPath());
	if(data2) {
		Log::get().writef("%2d:%2d(%-12.12s) -> %6d (%4d b) %#x (%s)\n",
				t->getTid(),pid,p ? p->getProgram() : "??",id,size2,this,getPath());
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
	if(EXPECT_FALSE(msg1 == NULL))
		return -ENOMEM;

	msg1->length = size1;
	msg1->id = id;
	if(EXPECT_TRUE(data1)) {
		Thread::addHeapAlloc(msg1);
		memcpy(msg1 + 1,data1,size1);
		Thread::remHeapAlloc(msg1);
	}

	if(EXPECT_FALSE(data2)) {
		msg2 = (Message*)Cache::alloc(sizeof(Message) + size2);
		if(EXPECT_FALSE(msg2 == NULL))
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
	if(EXPECT_FALSE(data2))
		msg2->thread = recipient;

	/* append to list */
	list->append(msg1);
	if(EXPECT_FALSE(msg2))
		list->append(msg2);

	/* notify the driver */
	if(list == &sendList) {
		VFSNode::acquireTree();
		if(EXPECT_TRUE(parent)) {
			static_cast<VFSDevice*>(parent)->addMsgs(1);
			if(EXPECT_FALSE(msg2))
				static_cast<VFSDevice*>(parent)->addMsgs(1);
			Sched::wakeup(EV_CLIENT,(evobj_t)parent);
		}
		VFSNode::releaseTree();
	}
	else {
		/* notify other possible waiters */
		if(recipient)
			recipient->unblock();
		else
			Sched::wakeup(EV_RECEIVED_MSG,(evobj_t)this);
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
		event = EV_CLIENT;
		list = &sendList;
		waitNode = parent;
	}
	else {
		event = EV_RECEIVED_MSG;
		list = &recvList;
		waitNode = this;
	}

	/* wait until a message arrives */
	SpinLock::acquire(&waitLock);
	while((msg = getMsg(t,list,flags)) == NULL) {
		if(EXPECT_FALSE(!block)) {
			SpinLock::release(&waitLock);
			return -EWOULDBLOCK;
		}
		/* if the channel has already been closed, there is no hope of success here */
		if(EXPECT_FALSE(closed || !isAlive())) {
			SpinLock::release(&waitLock);
			return -EDESTROYED;
		}
		t->wait(event,(evobj_t)waitNode);
		SpinLock::release(&waitLock);

		if(EXPECT_FALSE(ignoreSigs))
			Thread::switchNoSigs();
		else
			Thread::switchAway();

		if(EXPECT_FALSE(t->hasSignalQuick()))
			return -EINTR;
		/* if we waked up and there is no message, the driver probably died */
		if(EXPECT_FALSE(!isAlive()))
			return -EDESTROYED;
		SpinLock::acquire(&waitLock);
	}

	if(event == EV_CLIENT) {
		VFSNode::acquireTree();
		if(EXPECT_TRUE(parent))
			static_cast<VFSDevice*>(parent)->remMsgs(1);
		VFSNode::releaseTree();
		curClient = msg->thread;
	}
	SpinLock::release(&waitLock);

	if(EXPECT_FALSE(data && msg->length > size)) {
		Cache::free(msg);
		return -EINVAL;
	}

#if PRINT_MSGS
	Proc *p = Proc::getByPid(pid);
	Log::get().writef("%2d:%2d(%-12.12s) <- %6d (%4d b) %#x (%s)\n",
			t->getTid(),pid,p ? p->getProgram() : "??",msg->id,msg->length,this,getPath());
#endif

	/* copy data and id; since it may fail we have to ensure that our resources are free'd */
	Thread::addHeapAlloc(msg);
	if(EXPECT_TRUE(data))
		memcpy(data,msg + 1,msg->length);
	if(EXPECT_FALSE(id))
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
	os.writef("%-8s: snd=%zu rcv=%zu used=%d closed=%d fd=%02d shm=%zuK\n",
		name,sendList.length(),recvList.length(),used,closed,fd,shmem ? shmemSize / 1024 : 0);
	for(size_t i = 0; i < ARRAY_SIZE(lists); i++) {
		for(auto it = lists[i]->cbegin(); it != lists[i]->cend(); ++it) {
			os.writef("\t%s id=%u len=%zu, thread=%d:%d:%s\n",i == 0 ? "->" : "<-",
					it->id,it->length,
					it->thread ? it->thread->getTid() : -1,
					it->thread ? it->thread->getProc()->getPid() : -1,
					it->thread ? it->thread->getProc()->getProgram() : "");
		}
	}
}
