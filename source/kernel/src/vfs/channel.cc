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
#include <ipc/ipcbuf.h>
#include <ipc/proto/device.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define PRINT_MSGS			0

extern SpinLock waitLock;

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
	if(getParent()) {
		static_cast<VFSDevice*>(getParent())->remMsgs(sendList.length());
		static_cast<VFSDevice*>(getParent())->clientRemoved(this);
	}
	recvList.deleteAll();
	sendList.deleteAll();
}

int VFSChannel::isSupported(int op) const {
	if(!isAlive())
		return -EDESTROYED;
	/* if the driver doesn't implement read, its an error */
	if(!static_cast<VFSDevice*>(parent)->supports(op))
		return -ENOTSUP;
	return 0;
}

void VFSChannel::discardMsgs() {
	LockGuard<SpinLock> g(&waitLock);
	// remove from parent
	static_cast<VFSDevice*>(getParent())->remMsgs(sendList.length());

	// now clear lists
	sendList.deleteAll();
	recvList.deleteAll();
}

ssize_t VFSChannel::open(pid_t pid,const char *path,uint flags,int msgid) {
	char buffer[IPC_DEF_SIZE];
	ipc::IPCBuf ib(buffer,sizeof(buffer));
	ssize_t res = -ENOENT;
	OpenFile *clifile;
	Proc *p,*pp;
	pid_t ppid;
	Thread *t = Thread::getRunning();

	/* give the driver a file-descriptor for this new client; note that we have to do that
	 * immediatly because in close() we assume that the device has already one reference to it */
	VFSNode *par = getParent();
	ppid = par->getOwner();
	res = VFS::openFile(ppid,VFS_MSGS | VFS_DEVICE,this,getNo(),VFS_DEV_NO,&clifile);
	if(res < 0)
		return res;
	/* getByPid() is ok because if the parent-node exists, the owner does always exist, too */
	pp = Proc::getByPid(ppid);
	fd = FileDesc::assoc(pp,clifile);
	if(fd < 0) {
		clifile->close(ppid);
		return fd;
	}

	/* do we need to send an open to the driver? */
	res = isSupported(DEV_OPEN);
	if(res == -ENOTSUP)
		return 0;
	if(res < 0)
		goto error;

	p = Proc::getByPid(pid);
	assert(p != NULL);

	/* send msg to driver */
	ib << ipc::DevOpen::Request(flags,p->getEUid(),p->getEGid(),p->getPid(),ipc::CString(path));
	if(ib.error()) {
		res = -EINVAL;
		goto error;
	}
	res = send(pid,0,msgid,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		goto error;

	/* receive response */
	ib.reset();
	t->addResource();
	do
		res = receive(pid,0,NULL,ib.buffer(),ib.max(),true,false);
	while(res == -EINTR);
	t->remResource();
	if(res < 0)
		goto error;

	{
		ipc::DevOpen::Response r;
		ib >> r;
		if(r.res < 0) {
			res = r.res;
			goto error;
		}
		return r.res;
	}

error:
	/* the parent process might be dead now (in this case he would have already closed the file) */
	pp = Proc::getRef(ppid);
	if(pp) {
		/* the file might have been closed already */
		clifile = FileDesc::unassoc(pp,fd);
		if(clifile)
			clifile->close(ppid);
		Proc::relRef(pp);
	}
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
	char buffer[IPC_DEF_SIZE];
	ipc::IPCBuf ib(buffer,sizeof(buffer));

	/* send msg to fs */
	ssize_t res = send(pid,0,MSG_FS_ISTAT,NULL,0,NULL,0);
	if(res < 0)
		return res;

	/* receive response */
	do
		res = receive(pid,0,NULL,ib.buffer(),ib.max(),true,false);
	while(res == -EINTR);
	if(res < 0)
		return res;

	int err;
	ib >> err >> *info;
	if(err < 0)
		return err;
	if(ib.error())
		return -EINVAL;
	return 0;
}

static bool useSharedMem(const void *shmem,size_t shmsize,const void *buffer,size_t bufsize) {
	return shmem && (uintptr_t)buffer >= (uintptr_t)shmem &&
		(uintptr_t)buffer + bufsize > (uintptr_t)buffer &&
		(uintptr_t)buffer + bufsize <= (uintptr_t)shmem + shmsize;
}

ssize_t VFSChannel::read(pid_t pid,OpenFile *file,USER void *buffer,off_t offset,size_t count) {
	char ibuffer[IPC_DEF_SIZE];
	ipc::IPCBuf ib(ibuffer,sizeof(ibuffer));
	ssize_t res;

	if((res = isSupported(DEV_READ)) < 0)
		return res;

	/* if it's a char-device, block until there is data to read */
	VFSDevice *dev = static_cast<VFSDevice*>(getParent());
	if(S_ISCHR(dev->getMode())) {
		if(!file->shouldBlock()) {
			if(!dev->tryDown())
				return -EWOULDBLOCK;
		}
		else if(!dev->down())
			return -EINTR;
	}

	/* send msg to driver */
	Thread *t = Thread::getRunning();
	bool useshm = useSharedMem(shmem,shmemSize,buffer,count);
	ib << ipc::DevRead::Request(offset,count,useshm ? ((uintptr_t)buffer - (uintptr_t)shmem) : -1);
	res = file->sendMsg(pid,MSG_DEV_READ,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		return res;

	/* read response and ensure that we don't get killed until we've received both messages
	 * (otherwise the channel might get in an inconsistent state) */
	t->addResource();
	ib.reset();
	do {
		msgid_t mid;
		res = file->receiveMsg(pid,&mid,ib.buffer(),ib.max(),true);
		vassert(res < 0 || mid == MSG_DEV_READ_RESP,"mid=%u, res=%zd, node=%s:%p",
				mid,res,getPath(),this);
	}
	while(res == -EINTR);
	t->remResource();
	if(res < 0)
		return res;

	/* handle response */
	ipc::DevRead::Response r;
	ib >> r;
	if(r.res < 0)
		return r.res;

	if(!useshm && r.res > 0) {
		/* read data */
		t->addResource();
		do
			r.res = file->receiveMsg(pid,NULL,buffer,count,true);
		while(r.res == -EINTR);
		t->remResource();
	}
	return r.res;
}

ssize_t VFSChannel::write(pid_t pid,OpenFile *file,USER const void *buffer,off_t offset,size_t count) {
	char ibuffer[IPC_DEF_SIZE];
	ipc::IPCBuf ib(ibuffer,sizeof(ibuffer));
	ssize_t res;
	Thread *t = Thread::getRunning();
	bool useshm = useSharedMem(shmem,shmemSize,buffer,count);

	if((res = isSupported(DEV_WRITE)) < 0)
		return res;

	/* send msg and data to driver */
	ib << ipc::DevWrite::Request(offset,count,useshm ? ((uintptr_t)buffer - (uintptr_t)shmem) : -1);
	res = file->sendMsg(pid,MSG_DEV_WRITE,ib.buffer(),ib.pos(),useshm ? NULL : buffer,count);
	if(res < 0)
		return res;

	/* read response */
	ib.reset();
	t->addResource();
	do {
		msgid_t mid;
		res = file->receiveMsg(pid,&mid,ib.buffer(),ib.max(),true);
		vassert(res < 0 || mid == MSG_DEV_WRITE_RESP,"mid=%u, res=%zd, node=%s:%p",
				mid,res,getPath(),this);
	}
	while(res == -EINTR);
	t->remResource();
	if(res < 0)
		return res;

	ipc::DevWrite::Response r;
	ib >> r;
	return r.res;
}

int VFSChannel::sharefile(pid_t pid,OpenFile *file,const char *path,void *cliaddr,size_t size) {
	char ibuffer[IPC_DEF_SIZE];
	ipc::IPCBuf ib(ibuffer,sizeof(ibuffer));
	ssize_t res;
	Thread *t = Thread::getRunning();

	if(shmem != NULL)
		return -EEXIST;
	if((res = isSupported(DEV_SHFILE)) < 0)
		return res;

	/* send msg to driver */
	ib << ipc::DevShFile::Request(size,ipc::CString(path));
	if(ib.error())
		return -EINVAL;
	res = file->sendMsg(pid,MSG_DEV_SHFILE,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		return res;

	/* read response */
	ib.reset();
	t->addResource();
	do {
		msgid_t mid;
		res = file->receiveMsg(pid,&mid,ib.buffer(),ib.max(),true);
		vassert(res < 0 || mid == MSG_DEV_SHFILE_RESP,"mid=%u, res=%zd, node=%s:%p",
				mid,res,getPath(),this);
	}
	while(res == -EINTR);
	t->remResource();
	if(res < 0)
		return res;

	/* handle response */
	ipc::DevShFile::Response r;
	ib >> r;
	if(r.res < 0)
		return r.res;
	shmem = cliaddr;
	shmemSize = size;
	return 0;
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
	LockGuard<SpinLock> g(&waitLock);

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
		static_cast<VFSDevice*>(parent)->addMsgs(1);
		if(EXPECT_FALSE(msg2))
			static_cast<VFSDevice*>(parent)->addMsgs(1);
		Sched::wakeup(EV_CLIENT,(evobj_t)parent,false);
	}
	else {
		/* notify other possible waiters */
		if(recipient)
			recipient->unblock();
		else
			Sched::wakeup(EV_RECEIVED_MSG,(evobj_t)this,false);
	}
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
	waitLock.down();
	while((msg = getMsg(t,list,flags)) == NULL) {
		if(EXPECT_FALSE(!block)) {
			waitLock.up();
			return -EWOULDBLOCK;
		}
		/* if the channel has already been closed, there is no hope of success here */
		if(EXPECT_FALSE(closed || !isAlive())) {
			waitLock.up();
			return -EDESTROYED;
		}
		t->wait(event,(evobj_t)waitNode);
		waitLock.up();

		if(EXPECT_FALSE(ignoreSigs))
			Thread::switchNoSigs();
		else
			Thread::switchAway();

		if(EXPECT_FALSE(t->hasSignalQuick()))
			return -EINTR;
		/* if we waked up and there is no message, the driver probably died */
		if(EXPECT_FALSE(!isAlive()))
			return -EDESTROYED;
		waitLock.down();
	}

	if(event == EV_CLIENT) {
		static_cast<VFSDevice*>(parent)->remMsgs(1);
		curClient = msg->thread;
	}
	waitLock.up();

	if(EXPECT_FALSE(data && msg->length > size)) {
		Log::get().writef("INVALID: len=%zu, size=%zu\n",msg->length,size);
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
