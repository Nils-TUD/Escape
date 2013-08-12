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
#include <sys/task/proc.h>
#include <sys/mem/cache.h>
#include <sys/vfs/openfile.h>
#include <sys/vfs/fs.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/device.h>
#include <sys/vfs/vfs.h>
#include <sys/ostream.h>
#include <esc/messages.h>
#include <errno.h>

#define FILE_COUNT					(gftArray.getObjCount())

klock_t OpenFile::gftLock;
DynArray OpenFile::gftArray(sizeof(OpenFile),GFT_AREA,GFT_AREA_SIZE);
OpenFile *OpenFile::gftFreeList = NULL;
extern klock_t waitLock;

void OpenFile::decUsages() {
	SpinLock::acquire(&lock);
	assert(usageCount > 0);
	usageCount--;
	/* if it should be closed in the meanwhile, we have to close it now, because it wasn't possible
	 * previously because of our usage */
	if(usageCount == 0 && refCount == 0)
		doClose(Proc::getRunning());
	SpinLock::release(&lock);
}

int OpenFile::fcntl(A_UNUSED pid_t pid,uint cmd,int arg) {
	switch(cmd) {
		case F_GETACCESS:
			return flags & (VFS_READ | VFS_WRITE | VFS_MSGS);
		case F_GETFL:
			return flags & VFS_NOBLOCK;
		case F_SETFL:
			SpinLock::acquire(&lock);
			flags &= VFS_READ | VFS_WRITE | VFS_MSGS | VFS_CREATE | VFS_DEVICE;
			flags |= arg & VFS_NOBLOCK;
			SpinLock::release(&lock);
			return 0;
		case F_SETDATA: {
			VFSNode *n = node;
			int res = 0;
			SpinLock::acquire(&waitLock);
			if(devNo != VFS_DEV_NO || !IS_DEVICE(n->getMode()))
				res = -EINVAL;
			else
				res = static_cast<VFSDevice*>(n)->setReadable((bool)arg);
			SpinLock::release(&waitLock);
			return res;
		}
	}
	return -EINVAL;
}

int OpenFile::fstat(pid_t pid,USER sFileInfo *info) const {
	int res = 0;
	if(devNo == VFS_DEV_NO)
		node->getInfo(pid,info);
	else
		res = VFSFS::istat(pid,nodeNo,devNo,info);
	return res;
}

off_t OpenFile::seek(pid_t pid,off_t offset,uint whence) {
	off_t res;

	/* don't lock it during VFSFS::istat(). we don't need it in this case because position is
	 * simply set and never restored to oldPos */
	if(devNo == VFS_DEV_NO || whence != SEEK_END)
		SpinLock::acquire(&lock);

	off_t oldPos = position;
	if(devNo == VFS_DEV_NO) {
		res = node->seek(pid,position,offset,whence);
		if(res < 0) {
			SpinLock::release(&lock);
			return res;
		}
		position = res;
	}
	else {
		if(whence == SEEK_END) {
			sFileInfo info;
			res = VFSFS::istat(pid,nodeNo,devNo,&info);
			if(res < 0)
				return res;
			/* can't be < 0, therefore it will always be kept */
			position = info.size;
		}
		/* since the fs-device validates the position anyway we can simply set it */
		else if(whence == SEEK_SET)
			position = offset;
		else
			position += offset;
	}

	/* invalid position? */
	if(position < 0) {
		position = oldPos;
		res = -EINVAL;
	}
	else
		res = position;

	if(devNo == VFS_DEV_NO || whence != SEEK_END)
		SpinLock::release(&lock);
	return res;
}

ssize_t OpenFile::read(pid_t pid,USER void *buffer,size_t count) {
	if(!(flags & VFS_READ))
		return -EACCES;

	ssize_t readBytes;
	if(devNo == VFS_DEV_NO) {
		/* use the read-handler */
		readBytes = node->read(pid,this,buffer,position,count);
	}
	else {
		/* query the fs-device to read from the inode */
		readBytes = VFSFS::read(pid,nodeNo,devNo,buffer,position,count);
	}

	if(readBytes > 0) {
		SpinLock::acquire(&lock);
		position += readBytes;
		SpinLock::release(&lock);
	}

	if(readBytes > 0 && pid != KERNEL_PID) {
		Proc *p = Proc::getByPid(pid);
		/* no lock here because its not critical. we don't make decisions based on it or similar.
		 * its just for statistics. therefore, it doesn't really hurt if we add a bit less in
		 * very very rare cases. */
		p->stats.input += readBytes;
	}
	return readBytes;
}

ssize_t OpenFile::write(pid_t pid,USER const void *buffer,size_t count) {
	if(!(flags & VFS_WRITE))
		return -EACCES;

	ssize_t writtenBytes;
	if(devNo == VFS_DEV_NO) {
		/* write to the node */
		writtenBytes = node->write(pid,this,buffer,position,count);
	}
	else {
		/* query the fs-device to write to the inode */
		writtenBytes = VFSFS::write(pid,nodeNo,devNo,buffer,position,count);
	}

	if(writtenBytes > 0) {
		SpinLock::acquire(&lock);
		position += writtenBytes;
		SpinLock::release(&lock);
	}

	if(writtenBytes > 0 && pid != KERNEL_PID) {
		Proc *p = Proc::getByPid(pid);
		/* no lock; same reason as above */
		p->stats.output += writtenBytes;
	}
	return writtenBytes;
}

ssize_t OpenFile::sendMsg(pid_t pid,msgid_t id,USER const void *data1,size_t size1,
		USER const void *data2,size_t size2) {
	if(devNo != VFS_DEV_NO)
		return -EPERM;
	/* the device-messages (open, read, write, close) are always allowed */
	if(!IS_DEVICE_MSG(id) && !(flags & VFS_MSGS))
		return -EACCES;

	if(!IS_CHANNEL(node->getMode()))
		return -ENOTSUP;

	ssize_t err = static_cast<VFSChannel*>(node)->send(pid,flags,id,data1,size1,data2,size2);
	if(err == 0 && pid != KERNEL_PID) {
		Proc *p = Proc::getByPid(pid);
		/* no lock; same reason as above */
		p->stats.output += size1 + size2;
	}
	return err;
}

ssize_t OpenFile::receiveMsg(pid_t pid,USER msgid_t *id,USER void *data,size_t size,
		bool forceBlock) {
	if(devNo != VFS_DEV_NO)
		return -EPERM;

	if(!IS_CHANNEL(node->getMode()))
		return -ENOTSUP;

	ssize_t err = static_cast<VFSChannel*>(node)->receive(pid,flags,id,data,size,
			forceBlock || !(flags & VFS_NOBLOCK),forceBlock);
	if(err > 0 && pid != KERNEL_PID) {
		Proc *p = Proc::getByPid(pid);
		/* no lock; same reason as above */
		p->stats.input += err;
	}
	return err;
}

bool OpenFile::close(pid_t pid) {
	SpinLock::acquire(&lock);
	bool res = doClose(pid);
	SpinLock::release(&lock);
	return res;
}

bool OpenFile::doClose(pid_t pid) {
	/* decrement references; it may be already zero if we have closed the file previously but
	 * couldn't free it because there was still a user of it. */
	if(refCount > 0)
		refCount--;

	/* if there are no more references, free the file */
	if(refCount == 0) {
		/* if we have used a file-descriptor to get here, the usages are at least 1; otherwise it is
		 * 0, because it is used kernel-intern only and not passed to other "users". */
		if(usageCount <= 1) {
			if(devNo == VFS_DEV_NO)
				node->close(pid,this);
			/* VFSFS::close won't cause a context-switch; therefore we can keep the lock */
			else
				VFSFS::close(pid,nodeNo,devNo);

			/* mark unused */
			Cache::free(path);
			flags = 0;
			releaseFile(this);
			return true;
		}
	}
	return false;
}

int OpenFile::getClient(OpenFile *const *files,size_t count,size_t *index,VFSNode **client,uint flags) {
	Event::WaitObject waits[MAX_GETWORK_DEVICES];
	Thread *t = Thread::getRunning();
	bool inited = false;
	while(true) {
		SpinLock::acquire(&waitLock);
		int res = doGetClient(files,count,index,client);
		/* if we've found one or we shouldn't block, stop here */
		if(res != -ENOCLIENT || (flags & GW_NOBLOCK)) {
			SpinLock::release(&waitLock);
			return res;
		}

		/* build wait-objects */
		if(!inited) {
			for(size_t i = 0; i < count; i++) {
				waits[i].events = EV_CLIENT;
				waits[i].object = (evobj_t)files[i]->node;
			}
			inited = true;
		}

		/* wait for a client (accept signals) */
		Event::waitObjects(t,waits,count);
		SpinLock::release(&waitLock);

		Thread::switchAway();
		if(Signals::hasSignalFor(t->getTid()))
			return -EINTR;
	}
	return 0;
}

int OpenFile::doGetClient(OpenFile *const *files,size_t count,size_t *index,VFSNode **client) {
	VFSNode *match = NULL;
	for(size_t i = 0; i < count; i++) {
		const OpenFile *f = files[i];
		if(f->devNo != VFS_DEV_NO)
			return -EPERM;
		if(!IS_DEVICE(f->node->getMode()))
			return -EPERM;
	}

	bool retry,cont = true;
	do {
		retry = false;
		for(size_t i = 0; cont && i < count; i++) {
			const OpenFile *f = files[i];
			VFSNode *node = f->node;

			*client = static_cast<VFSDevice*>(node)->getWork(&cont,&retry);
			if(*client) {
				if(match)
					match->unref();
				if(index)
					*index = i;
				if(cont) {
					match = *client;
				}
				else {
					static_cast<VFSChannel*>(*client)->setUsed(true);
					return 0;
				}
			}
		}
		/* if we have a match, use this one */
		if(match) {
			static_cast<VFSChannel*>(match)->setUsed(true);
			*client = match;
			return 0;
		}
		/* if not and we've skipped a client, try another time */
	}
	while(retry);
	return -ENOCLIENT;
}

int OpenFile::openClient(pid_t pid,inode_t clientId,OpenFile **cfile) {
	bool isValid;
	/* search for the client */
	const VFSNode *n = node->openDir(true,&isValid);
	if(isValid) {
		while(n != NULL) {
			if(n->getNo() == clientId)
				break;
			n = n->next;
		}
	}
	node->closeDir(true);
	if(n == NULL)
		return -ENOENT;

	/* open file */
	return VFS::openFile(pid,VFS_MSGS | VFS_DEVICE,n,n->getNo(),VFS_DEV_NO,cfile);
}

void OpenFile::print(OStream &os) const {
	os.writef("%3d [ %2u refs, %2u uses (%u:%u",
			gftArray.getIndex(this),refCount,usageCount,devNo,nodeNo);
	if(devNo == VFS_DEV_NO && VFSNode::isValid(nodeNo))
		os.writef(":%s)",node->getPath());
	else
		os.writef(")");
	os.writef(" ]");
}

void OpenFile::printAll(OStream &os) {
	os.writef("Global File Table:\n");
	for(size_t i = 0; i < FILE_COUNT; i++) {
		OpenFile *f = (OpenFile*)gftArray.getObj(i);
		if(f->flags != 0) {
			os.writef("\tfile @ index %d\n",i);
			os.writef("\t\tflags: ");
			if(f->flags & VFS_READ)
				os.writef("READ ");
			if(f->flags & VFS_WRITE)
				os.writef("WRITE ");
			if(f->flags & VFS_NOBLOCK)
				os.writef("NOBLOCK ");
			if(f->flags & VFS_DEVICE)
				os.writef("DEVICE ");
			if(f->flags & VFS_MSGS)
				os.writef("MSGS ");
			os.writef("\n");
			os.writef("\t\tnodeNo: %d\n",f->nodeNo);
			os.writef("\t\tdevNo: %d\n",f->devNo);
			os.writef("\t\tpos: %Od\n",f->position);
			os.writef("\t\trefCount: %d\n",f->refCount);
			if(f->owner == KERNEL_PID)
				os.writef("\t\towner: %d (kernel)\n",f->owner);
			else {
				const Proc *p = Proc::getByPid(f->owner);
				os.writef("\t\towner: %d:%s\n",f->owner,p ? p->getCommand() : "???");
			}
			if(f->devNo == VFS_DEV_NO) {
				VFSNode *n = f->node;
				if(!n->isAlive())
					os.writef("\t\tFile: <destroyed>\n");
				else
					os.writef("\t\tFile: '%s'\n",f->getPath());
			}
			else
				os.writef("\t\tFile: '%s'\n",f->getPath());
		}
	}
}

size_t OpenFile::getCount() {
	size_t count = 0;
	for(size_t i = 0; i < FILE_COUNT; i++) {
		OpenFile *f = (OpenFile*)gftArray.getObj(i);
		if(f->flags != 0)
			count++;
	}
	return count;
}

int OpenFile::getFree(pid_t pid,ushort flags,inode_t nodeNo,dev_t devNo,const VFSNode *n,OpenFile **f) {
	const uint userFlags = VFS_READ | VFS_WRITE | VFS_MSGS | VFS_NOBLOCK | VFS_DEVICE | VFS_EXCLUSIVE;
	size_t i;
	bool isDrvUse = false;
	OpenFile *e;
	/* ensure that we don't increment usages of an unused slot */
	assert(flags & (VFS_READ | VFS_WRITE | VFS_MSGS));
	assert(!(flags & ~userFlags));

	if(devNo == VFS_DEV_NO) {
		/* we can add pipes here, too, since every open() to a pipe will get a new node anyway */
		isDrvUse = (n->getMode() & (MODE_TYPE_CHANNEL | MODE_TYPE_PIPE)) ? true : false;
	}
	/* doesn't work for channels and pipes */
	if(isDrvUse && (flags & VFS_EXCLUSIVE))
		return -EINVAL;

	SpinLock::acquire(&gftLock);
	/* for devices it doesn't matter whether we use an existing file or a new one, because it is
	 * no problem when multiple threads use it for writing */
	if(!isDrvUse) {
		/* TODO walk through used-list and pick first from freelist */
		ushort rwFlags = flags & userFlags;
		for(i = 0; i < FILE_COUNT; i++) {
			e = (OpenFile*)gftArray.getObj(i);
			/* used slot and same node? */
			if(e->flags != 0) {
				/* same file? */
				if(e->devNo == devNo && e->nodeNo == nodeNo) {
					if(e->owner == pid) {
						/* if the flags are the same we don't need a new file */
						if((e->flags & userFlags) == rwFlags) {
							e->incRefs();
							SpinLock::release(&gftLock);
							*f = e;
							return 0;
						}
					}
					else if((rwFlags & VFS_EXCLUSIVE) || (e->flags & VFS_EXCLUSIVE)) {
						SpinLock::release(&gftLock);
						return -EBUSY;
					}
				}
			}
		}
	}

	/* if there is no free slot anymore, extend our dyn-array */
	if(gftFreeList == NULL) {
		size_t j;
		i = gftArray.getObjCount();
		if(!gftArray.extend()) {
			SpinLock::release(&gftLock);
			return -ENFILE;
		}
		/* put all except i on the freelist */
		for(j = i + 1; j < gftArray.getObjCount(); j++) {
			e = (OpenFile*)gftArray.getObj(j);
			e->next = gftFreeList;
			gftFreeList = e;
		}
		*f = e = (OpenFile*)gftArray.getObj(i);
	}
	else {
		/* use the first from the freelist */
		e = gftFreeList;
		gftFreeList = gftFreeList->next;
		*f = e;
	}
	SpinLock::release(&gftLock);

	/* count references of virtual nodes */
	if(devNo == VFS_DEV_NO) {
		n->increaseRefs();
		e->node = const_cast<VFSNode*>(n);
	}
	else
		e->node = NULL;
	e->owner = pid;
	e->flags = flags;
	e->refCount = 1;
	e->usageCount = 0;
	e->position = 0;
	e->devNo = devNo;
	e->nodeNo = nodeNo;
	e->path = NULL;
	return 0;
}

void OpenFile::releaseFile(OpenFile *file) {
	SpinLock::acquire(&gftLock);
	file->next = gftFreeList;
	gftFreeList = file;
	SpinLock::release(&gftLock);
}
