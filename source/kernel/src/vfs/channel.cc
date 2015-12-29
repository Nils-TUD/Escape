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

#include <esc/ipc/ipcbuf.h>
#include <esc/proto/file.h>
#include <mem/cache.h>
#include <mem/useraccess.h>
#include <mem/virtmem.h>
#include <sys/messages.h>
#include <task/filedesc.h>
#include <task/proc.h>
#include <task/thread.h>
#include <vfs/channel.h>
#include <vfs/device.h>
#include <vfs/node.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <assert.h>
#include <common.h>
#include <errno.h>
#include <log.h>
#include <spinlock.h>
#include <string.h>
#include <video.h>

#define PRINT_MSGS			0

extern SpinLock waitLock;

uint16_t VFSChannel::nextRid = 1;

VFSChannel::VFSChannel(pid_t pid,VFSNode *p,bool &success)
		/* permissions are basically irrelevant here since the userland can't open a channel directly. */
		/* but in order to allow devices to be created by non-root users, give permissions for everyone */
		/* otherwise, if root uses that device, the driver is unable to open this channel. */
		: VFSNode(pid,generateId(pid),MODE_TYPE_CHANNEL | 0777,success), fd(-1),
		  handler(static_cast<VFSDevice*>(p)->getCreator()), closed(false),
		  shmem(NULL), shmemSize(0), sendList(), recvList() {
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

int VFSChannel::openForDriver() {
	OpenFile *clifile;
	VFSNode *par = getParent();
	pid_t ppid = par->getOwner();
	int res = VFS::openFile(ppid,VFS_MSGS | VFS_DEVICE,this,getNo(),VFS_DEV_NO,&clifile);
	if(res < 0)
		return res;
	/* getByPid() is ok because if the parent-node exists, the owner does always exist, too */
	Proc *pp = Proc::getByPid(ppid);
	fd = FileDesc::assoc(pp,clifile);
	if(fd < 0) {
		clifile->close(ppid);
		return fd;
	}
	return 0;
}

void VFSChannel::closeForDriver() {
	/* the parent process might be dead now (in this case he would have already closed the file) */
	VFSNode *par = getParent();
	pid_t ppid = par->getOwner();
	Proc *pp = Proc::getRef(ppid);
	if(pp) {
		/* the file might have been closed already */
		OpenFile *clifile = FileDesc::unassoc(pp,fd);
		if(clifile)
			clifile->close(ppid);
		Proc::relRef(pp);
	}
}

ssize_t VFSChannel::open(pid_t pid,const char *path,uint flags,int msgid,mode_t mode) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));
	ssize_t res;
	Proc *p;
	msgid_t mid;

	/* give the driver a file-descriptor for this new client; note that we have to do that
	 * immediatly because in close() we assume that the device has already one reference to it */
	res = openForDriver();
	if(res < 0)
		return res;

	/* do we need to send an open to the driver? */
	res = isSupported(DEV_OPEN);
	if(res == -ENOTSUP)
		return 0;
	if(res < 0)
		goto error;

	p = Proc::getByPid(pid);
	assert(p != NULL);

	/* send msg to driver */
	ib << esc::FileOpen::Request(flags,fs::User(p->getEUid(),p->getEGid(),p->getPid()),
		esc::CString(path),mode);
	if(ib.error()) {
		res = -EINVAL;
		goto error;
	}
	res = send(pid,0,msgid,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		goto error;

	/* receive response */
	ib.reset();
	mid = res;
	res = receive(pid,0,&mid,ib.buffer(),ib.max());
	if(res < 0)
		goto error;

	{
		esc::FileOpen::Response r;
		ib >> r;
		if(r.err < 0) {
			res = r.err;
			goto error;
		}
		return r.res;
	}

error:
	closeForDriver();
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

ssize_t VFSChannel::getSize(pid_t pid) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	if(isSupported(DEV_SIZE) < 0)
		return 0;

	/* send msg to device */
	ssize_t res = send(pid,0,MSG_FILE_SIZE,NULL,0,NULL,0);
	if(res < 0)
		return res;

	/* receive response */
	msgid_t mid = res;
	res = receive(pid,0,&mid,ib.buffer(),ib.max());
	if(res < 0)
		return res;

	ib >> res;
	return res;
}

static bool useSharedMem(const void *shmem,size_t shmsize,const void *buffer,size_t bufsize) {
	return shmem && (uintptr_t)buffer >= (uintptr_t)shmem &&
		(uintptr_t)buffer + bufsize > (uintptr_t)buffer &&
		(uintptr_t)buffer + bufsize <= (uintptr_t)shmem + shmsize;
}

uint VFSChannel::getReceiveFlags() const {
	uint flags = 0;
	/* allow signals if either the cancel message or cancel signal is supported */
	if(isSupported(DEV_CANCEL | DEV_CANCELSIG) == 0)
		flags |= VFS_SIGNALS;
	/* but enforce blocking if the cancel message is not supported since we have to leave the
	 * channel in a consistent state */
	if(isSupported(DEV_CANCEL) < 0)
		flags |= VFS_BLOCK;
	return flags;
}

ssize_t VFSChannel::read(pid_t pid,OpenFile *file,USER void *buffer,off_t offset,size_t count) {
	ulong ibuffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(ibuffer,sizeof(ibuffer));
	ssize_t res;

	if((res = isSupported(DEV_READ)) < 0)
		return res;

	/* send msg to driver */
	bool useshm = useSharedMem(shmem,shmemSize,buffer,count);
	ib << esc::FileRead::Request(offset,count,useshm ? ((uintptr_t)buffer - (uintptr_t)shmem) : -1);
	res = file->sendMsg(pid,MSG_FILE_READ,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		return res;

	msgid_t mid = res;
	uint flags = getReceiveFlags();
	while(1) {
		/* read response and ensure that we don't get killed until we've received both messages
		 * (otherwise the channel might get in an inconsistent state) */
		ib.reset();
		res = file->receiveMsg(pid,&mid,ib.buffer(),ib.max(),flags);
		if(res < 0) {
			if(res == -EINTR || res == -EWOULDBLOCK) {
				int cancelRes = cancel(pid,file,mid);
				if(cancelRes == 1) {
					/* if the result is already there, get it, but don't allow signals anymore
					 * and force blocking */
					flags = VFS_BLOCK;
					continue;
				}
			}
			return res;
		}

		/* handle response */
		esc::FileRead::Response r;
		ib >> r;
		if(r.err < 0)
			return r.err;

		/* read data */
		if(!useshm && r.res > 0)
			r.res = file->receiveMsg(pid,&mid,buffer,count,0);
		return r.res;
	}
	A_UNREACHED;
}

ssize_t VFSChannel::write(pid_t pid,OpenFile *file,USER const void *buffer,off_t offset,size_t count) {
	ulong ibuffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(ibuffer,sizeof(ibuffer));
	ssize_t res;
	bool useshm = useSharedMem(shmem,shmemSize,buffer,count);

	if((res = isSupported(DEV_WRITE)) < 0)
		return res;

	/* send msg and data to driver */
	ib << esc::FileWrite::Request(offset,count,useshm ? ((uintptr_t)buffer - (uintptr_t)shmem) : -1);
	res = file->sendMsg(pid,MSG_FILE_WRITE,ib.buffer(),ib.pos(),useshm ? NULL : buffer,count);
	if(res < 0)
		return res;

	msgid_t mid = res;
	uint flags = getReceiveFlags();
	while(1) {
		/* read response */
		ib.reset();
		res = file->receiveMsg(pid,&mid,ib.buffer(),ib.max(),flags);
		if(res < 0) {
			if(res == -EINTR || res == -EWOULDBLOCK) {
				int cancelRes = cancel(pid,file,mid);
				if(cancelRes == 1) {
					/* if the result is already there, get it, but don't allow signals anymore
					 * and force blocking */
					flags = VFS_BLOCK;
					continue;
				}
			}
			return res;
		}

		esc::FileWrite::Response r;
		ib >> r;
		if(r.err < 0)
			return r.err;
		return r.res;
	}
	A_UNREACHED;
}

int VFSChannel::cancel(pid_t pid,OpenFile *file,msgid_t mid) {
	ulong ibuffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(ibuffer,sizeof(ibuffer));

	/* send SIGCANCEL to the handling thread of this channel (this might be dead; so use getRef) */
	bool sent = false;
	if(isSupported(DEV_CANCELSIG) == 0) {
		Thread *t = Thread::getRef(handler);
		if(t) {
			sent = Signals::addSignalFor(t,SIGCANCEL);
			Thread::relRef(t);
		}
	}

	int res;
	if((res = isSupported(DEV_CANCEL)) < 0)
		return sent ? 0 : res;

	ib << mid;
	res = file->sendMsg(pid,MSG_DEV_CANCEL,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		return res;

	ib.reset();
	mid = res;
	res = file->receiveMsg(pid,&mid,ib.buffer(),ib.max(),VFS_BLOCK);
	if(res < 0)
		return res;

	/* handle response */
	ib >> res;
	return res;
}

int VFSChannel::sharefile(pid_t pid,OpenFile *file,const char *path,void *cliaddr,size_t size) {
	ulong ibuffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(ibuffer,sizeof(ibuffer));
	ssize_t res;

	if(shmem != NULL)
		return -EEXIST;
	if((res = isSupported(DEV_SHFILE)) < 0)
		return res;

	/* send msg to driver */
	ib << esc::FileShFile::Request(size,esc::CString(path));
	if(ib.error())
		return -EINVAL;
	res = file->sendMsg(pid,MSG_DEV_SHFILE,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		return res;

	/* read response */
	ib.reset();
	msgid_t mid = res;
	res = file->receiveMsg(pid,&mid,ib.buffer(),ib.max(),0);
	if(res < 0)
		return res;

	/* handle response */
	esc::FileShFile::Response r;
	ib >> r;
	if(r.err < 0)
		return r.err;
	shmem = cliaddr;
	shmemSize = size;
	return 0;
}

int VFSChannel::creatsibl(pid_t pid,OpenFile *file,VFSChannel *sibl,int arg) {
	ulong ibuffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(ibuffer,sizeof(ibuffer));
	msgid_t mid;
	uint flags;

	int res;
	if((res = isSupported(DEV_CREATSIBL)) < 0)
		return res;

	/* first, give the driver a file-descriptor for the new channel */
	res = sibl->openForDriver();
	if(res < 0)
		return res;

	/* send msg to driver */
	ib << esc::FileCreatSibl::Request(sibl->fd,arg);
	res = file->sendMsg(pid,MSG_DEV_CREATSIBL,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		goto error;

	mid = res;
	flags = getReceiveFlags();
	while(1) {
		/* read response */
		ib.reset();
		res = file->receiveMsg(pid,&mid,ib.buffer(),ib.max(),flags);
		if(res < 0) {
			if(res == -EINTR || res == -EWOULDBLOCK) {
				int cancelRes = cancel(pid,file,mid);
				if(cancelRes == 1) {
					/* if the result is already there, get it, but don't allow signals anymore
					 * and force blocking */
					flags = VFS_BLOCK;
					continue;
				}
			}
			goto error;
		}

		esc::FileCreatSibl::Response r;
		ib >> r;
		if(r.err < 0) {
			res = r.err;
			goto error;
		}
		return 0;
	}
	A_UNREACHED;

error:
	sibl->closeForDriver();
	return res;
}

ssize_t VFSChannel::send(A_UNUSED pid_t pid,ushort flags,msgid_t id,USER const void *data1,
                         size_t size1,USER const void *data2,size_t size2) {
	esc::SList<Message> *list;
	Message *msg1,*msg2 = NULL;
	int res;

	/* devices write to the receive-list (which will be read by other processes) */
	if(flags & VFS_DEVICE) {
		assert(data2 == NULL && size2 == 0);
		list = &recvList;
	}
	/* other processes write to the send-list (which will be read by the driver) */
	else
		list = &sendList;

	/* create message and copy data to it */
	msg1 = new (size1) Message(size1);
	if(EXPECT_FALSE(msg1 == NULL))
		return -ENOMEM;

	if(EXPECT_TRUE(data1)) {
		if(EXPECT_FALSE((res = UserAccess::read(msg1 + 1,data1,size1)) < 0))
			goto errorMsg1;
	}

	if(EXPECT_FALSE(data2)) {
		msg2 = new (size2) Message(size2);
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
		LockGuard<SpinLock> g(&waitLock);

		/* set request id. for clients, we generate a new unique request-id */
		if(list == &sendList) {
			id &= 0xFFFF;
			/* prevent to set the MSB. otherwise the return-value would be negative (on 32-bit) */
			id |= ((nextRid++) & 0x7FFF) << 16;
			/* it can't be 0. this is a special value */
			if(id >> 16 == 0)
				id |= 0x00010000;
		}
		/* for devices, we just use whatever the driver gave us */
		msg1->id = id;
		if(EXPECT_FALSE(data2))
			msg2->id = id;

		/* append to list */
		list->append(msg1);
		if(EXPECT_FALSE(msg2))
			list->append(msg2);

		/* notify the driver */
		if(list == &sendList) {
			static_cast<VFSDevice*>(parent)->addMsgs(1);
			if(EXPECT_FALSE(msg2))
				static_cast<VFSDevice*>(parent)->addMsgs(1);
			Sched::wakeup(EV_CLIENT,(evobj_t)parent,true);
		}
		else {
			/* notify other possible waiters */
			Sched::wakeup(EV_RECEIVED_MSG,(evobj_t)this,true);
		}
	}

#if PRINT_MSGS
	{
		Thread *t = Thread::getRunning();
		Proc *p = Proc::getByPid(pid);
		Log::get().writef("%2d:%2d(%-12.12s) -> %5u:%5u (%4d b) %#x (%s)\n",
				t->getTid(),pid,p ? p->getProgram() : "??",id >> 16,id & 0xFFFF,size1,this,getPath());
		if(data2) {
			Log::get().writef("%2d:%2d(%-12.12s) -> %5u:%5u (%4d b) %#x (%s)\n",
					t->getTid(),pid,p ? p->getProgram() : "??",id >> 16,id & 0xFFFF,size2,this,getPath());
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

ssize_t VFSChannel::receive(A_UNUSED pid_t pid,ushort flags,msgid_t *id,USER void *data,size_t size) {
	esc::SList<Message> *list;
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
	while((msg = getMsg(list,*id,flags)) == NULL) {
		if(EXPECT_FALSE((flags & (VFS_NOBLOCK | VFS_BLOCK)) == VFS_NOBLOCK)) {
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

		if(flags & VFS_SIGNALS) {
			Thread::switchAway();
			if(EXPECT_FALSE(t->hasSignal()))
				return -EINTR;
		}
		else
			Thread::switchNoSigs();

		/* if we waked up and there is no message, the driver probably died */
		if(EXPECT_FALSE(!isAlive()))
			return -EDESTROYED;
		waitLock.down();
	}

	if(event == EV_CLIENT)
		static_cast<VFSDevice*>(parent)->remMsgs(1);
	waitLock.up();

#if PRINT_MSGS
	Proc *p = Proc::getByPid(pid);
	Log::get().writef("%2d:%2d(%-12.12s) <- %5u:%5u (%4d b) %#x (%s)\n",
			t->getTid(),pid,p ? p->getProgram() : "??",msg->id >> 16,msg->id & 0xFFFF,
			msg->length,this,getPath());
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

VFSChannel::Message *VFSChannel::getMsg(esc::SList<Message> *list,msgid_t mid,ushort flags) {
	/* drivers get always the first message */
	if(flags & VFS_DEVICE)
		return list->removeFirst();

	/* find the message for the given id */
	Message *p = NULL;
	for(auto it = list->begin(); it != list->end(); p = &*it, ++it) {
		/* either it's a "for anybody" message, or the id has to match */
		if(it->id == mid || (it->id >> 16) == 0) {
			list->removeAt(p,&*it);
			return &*it;
		}
	}
	return NULL;
}

void VFSChannel::print(OStream &os) const {
	const esc::SList<Message> *lists[] = {&sendList,&recvList};
	os.writef("%-8s: snd=%zu rcv=%zu closed=%d handler=%d fd=%02d shm=%zuK\n",
		name,sendList.length(),recvList.length(),closed,handler,fd,shmem ? shmemSize / 1024 : 0);
	for(size_t i = 0; i < ARRAY_SIZE(lists); i++) {
		for(auto it = lists[i]->cbegin(); it != lists[i]->cend(); ++it) {
			os.writef("\t%s id=%u:%u len=%zu\n",i == 0 ? "->" : "<-",
				it->id >> 16,it->id & 0xFFFF,it->length);
		}
	}
}
