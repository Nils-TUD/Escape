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
#include <mem/cache.h>
#include <sys/messages.h>
#include <task/proc.h>
#include <vfs/channel.h>
#include <vfs/device.h>
#include <vfs/fs.h>
#include <vfs/dir.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <common.h>
#include <errno.h>
#include <ostream.h>

SpinLock OpenFile::gftLock;
DynArray OpenFile::gftArray(sizeof(OpenFile),GFT_AREA,GFT_AREA_SIZE);
OpenFile *OpenFile::usedList = NULL;
OpenFile *OpenFile::exclList = NULL;
OpenFile *OpenFile::gftFreeList = NULL;
SpinLock OpenFile::semLock;
esc::Treap<OpenFile::SemTreapNode> OpenFile::sems;
extern SpinLock waitLock;

void OpenFile::decUsages() {
	LockGuard<SpinLock> g(&lock);
	assert(usageCount > 0);
	usageCount--;
	/* if it should be closed in the meanwhile, we have to close it now, because it wasn't possible
	 * previously because of our usage */
	if(EXPECT_FALSE(usageCount == 0 && refCount == 0))
		doClose();
}

int OpenFile::fcntl(uint cmd,int arg) {
	switch(cmd) {
		case F_GETACCESS:
			return flags & (VFS_READ | VFS_WRITE | VFS_MSGS);

		case F_GETFL:
			return flags & VFS_NOBLOCK;

		case F_SETFL: {
			LockGuard<SpinLock> g(&lock);
			flags &= VFS_READ | VFS_WRITE | VFS_MSGS | VFS_CREATE | VFS_DEVICE;
			flags |= arg & VFS_NOBLOCK;
			return 0;
		}

		case F_SEMUP:
		case F_SEMDOWN: {
			if(sem == NULL) {
				LockGuard<SpinLock> g(&semLock);
				if(sem == NULL) {
					FileId id(devNo,nodeNo);
					sem = sems.find(id);
					if(sem == NULL) {
						sem = new SemTreapNode(id);
						sems.insert(sem);
					}
					sem->refs++;
				}
			}

			if(cmd == F_SEMUP)
				sem->sem.up();
			else {
				Thread *t = Thread::getRunning();
				sem->sem.down(true);
				if(t->hasSignal())
					return -EINTR;
			}
			return 0;
		}
	}
	return -EINVAL;
}

int OpenFile::fstat(struct stat *info) const {
	int res = 0;
	if(devNo == VFS_DEV_NO)
		node->getInfo(info);
	else if(IS_CHANNEL(node->getMode())) {
		VFSChannel *chan = static_cast<VFSChannel*>(node);
		res = VFSFS::fstat(chan,info);
	}
	else
		res = -EINVAL;
	return res;
}

int OpenFile::chmod(mode_t mode) {
	if(~mntperm & VFS_WRITE)
		return -EPERM;

	int err = -EINVAL;
	if(devNo == VFS_DEV_NO)
		err = node->chmod(user,mode);
	else if(IS_CHANNEL(node->getMode())) {
		VFSChannel *chan = static_cast<VFSChannel*>(node);
		err = VFSFS::chmod(chan,mode);
	}
	return err;
}

int OpenFile::chown(uid_t uid,gid_t gid) {
	if(~mntperm & VFS_WRITE)
		return -EPERM;

	int err = -EINVAL;
	if(devNo == VFS_DEV_NO)
		err = node->chown(user,uid,gid);
	else if(IS_CHANNEL(node->getMode())) {
		VFSChannel *chan = static_cast<VFSChannel*>(node);
		err = VFSFS::chown(chan,uid,gid);
	}
	return err;
}

int OpenFile::utime(const struct utimbuf *utimes) {
	if(~mntperm & VFS_WRITE)
		return -EPERM;

	int err = -EINVAL;
	if(devNo == VFS_DEV_NO)
		err = node->utime(user,utimes);
	else if(IS_CHANNEL(node->getMode())) {
		VFSChannel *chan = static_cast<VFSChannel*>(node);
		err = VFSFS::utime(chan,utimes);
	}
	return err;
}

int OpenFile::link(OpenFile *dir,const char *name) {
	if(devNo != dir->devNo)
		return -EXDEV;
	if(!(dir->flags & VFS_WRITE))
		return -EACCES;

	int err = -EINVAL;
	if(devNo == VFS_DEV_NO)
		err = -ENOTSUP;
	else if(IS_CHANNEL(node->getMode())) {
		VFSChannel *targetChan = static_cast<VFSChannel*>(node);
		VFSChannel *dirChan = static_cast<VFSChannel*>(dir->node);
		err = VFSFS::link(targetChan,dirChan,name);
	}
	return err;
}

int OpenFile::unlink(const char *name) {
	if(!(flags & VFS_WRITE))
		return -EACCES;

	int err = -EINVAL;
	if(devNo == VFS_DEV_NO)
		err = node->unlink(user,name);
	else if(IS_CHANNEL(node->getMode())) {
		VFSChannel *chan = static_cast<VFSChannel*>(node);
		err = VFSFS::unlink(chan,name);
	}
	return err;
}

int OpenFile::rename(const char *oldName,OpenFile *newDir,const char *newName) {
	if(devNo != newDir->devNo)
		return -EXDEV;
	if(!(flags & VFS_WRITE) || !(newDir->flags & VFS_WRITE))
		return -EACCES;

	int err = -EINVAL;
	if(devNo == VFS_DEV_NO)
		err = node->rename(user,oldName,newDir->node,newName);
	else if(IS_CHANNEL(node->getMode()) && IS_CHANNEL(newDir->node->getMode())) {
		VFSChannel *oldChan = static_cast<VFSChannel*>(node);
		VFSChannel *newChan = static_cast<VFSChannel*>(newDir->node);
		err = VFSFS::rename(oldChan,oldName,newChan,newName);
	}
	return err;
}

int OpenFile::mkdir(const char *name,mode_t mode) {
	if(!(flags & VFS_WRITE))
		return -EACCES;

	int err = -EINVAL;
	if(devNo == VFS_DEV_NO)
		err = node->mkdir(user,name,mode);
	else if(IS_CHANNEL(node->getMode())) {
		VFSChannel *chan = static_cast<VFSChannel*>(node);
		err = VFSFS::mkdir(chan,name,S_IFDIR | (mode & MODE_PERM));
	}
	return err;
}

int OpenFile::rmdir(const char *name) {
	if(!(flags & VFS_WRITE))
		return -EACCES;

	int err = -EINVAL;
	if(devNo == VFS_DEV_NO)
		err = node->rmdir(user,name);
	else if(IS_CHANNEL(node->getMode())) {
		VFSChannel *chan = static_cast<VFSChannel*>(node);
		err = VFSFS::rmdir(chan,name);
	}
	return err;
}

int OpenFile::symlink(const char *name,const char *target) {
	if(!(flags & VFS_WRITE))
		return -EACCES;

	int err = -EINVAL;
	if(devNo == VFS_DEV_NO)
		err = node->symlink(user,name,target);
	else if(IS_CHANNEL(node->getMode())) {
		VFSChannel *chan = static_cast<VFSChannel*>(node);
		err = VFSFS::symlink(chan,name,target);
	}
	return err;
}

int OpenFile::createdev(const char *name,mode_t mode,uint type,uint ops,OpenFile **file) {
	if(devNo != VFS_DEV_NO)
		return -ENOTSUP;
	if(!(flags & VFS_WRITE))
		return -EACCES;

	VFSNode *srv;
	int res = node->createdev(user,name,mode,type,ops,&srv);
	if(res < 0)
		return res;

	res = VFS::openFile(user,mntperm,VFS_DEVICE,srv,srv->getNo(),VFS_DEV_NO,file);
	/* if an error occurred, release it twice to destroy the node */
	if(res < 0)
		VFSNode::release(srv);
	VFSNode::release(srv);
	return res;
}

int OpenFile::createchan(uint perm,OpenFile **chan) {
	if(!IS_DEVICE(node->getMode()))
		return -ENOTSUP;
	if(!(flags & VFS_DEVICE))
		return -EPERM;

	/* create new channel */
	VFSChannel *chnode = createObj<VFSChannel>(user,node);
	if(chnode == NULL)
		return -ENOMEM;

	/* open new file for the channel */
	int res = VFS::openFile(user,0,VFS_DEVICE | perm,chnode,chnode->getNo(),VFS_DEV_NO,chan);
	if(res < 0) {
		VFSNode::release(chnode);
		VFSNode::release(chnode);
		return res;
	}

	/* the file holds a reference now */
	VFSNode::release(chnode);
	return 0;
}

off_t OpenFile::seek(off_t offset,uint whence) {
	off_t res;

	/* don't lock it during VFSFS::istat(). we don't need it in this case because position is
	 * simply set and never restored to oldPos */
	if(devNo == VFS_DEV_NO || whence != SEEK_END)
		lock.down();

	off_t oldPos = position;
	if(devNo == VFS_DEV_NO) {
		res = node->seek(position,offset,whence);
		if(EXPECT_FALSE(res < 0)) {
			lock.up();
			return res;
		}
		position = res;
	}
	else {
		if(whence == SEEK_END) {
			struct stat info;
			res = fstat(&info);
			if(EXPECT_FALSE(res < 0))
				return res;
			/* can't be < 0, therefore it will always be kept */
			position = info.st_size;
		}
		/* since the fs-device validates the position anyway we can simply set it */
		else if(whence == SEEK_SET)
			position = offset;
		else
			position += offset;
	}

	/* invalid position? */
	if(EXPECT_FALSE(position < 0)) {
		position = oldPos;
		res = -EINVAL;
	}
	else
		res = position;

	if(devNo == VFS_DEV_NO || whence != SEEK_END)
		lock.up();
	return res;
}

ssize_t OpenFile::read(USER void *buffer,size_t count) {
	if(EXPECT_FALSE(!(flags & VFS_READ)))
		return -EACCES;

	/* use the read-handler */
	ssize_t readBytes = node->read(this,buffer,position,count);
	if(EXPECT_TRUE(readBytes > 0)) {
		LockGuard<SpinLock> g(&lock);
		position += readBytes;
	}

	return readBytes;
}

ssize_t OpenFile::write(USER const void *buffer,size_t count) {
	if(EXPECT_FALSE(!(flags & VFS_WRITE)))
		return -EACCES;

	/* write to the node */
	ssize_t writtenBytes = node->write(this,buffer,position,count);
	if(EXPECT_TRUE(writtenBytes > 0)) {
		LockGuard<SpinLock> g(&lock);
		position += writtenBytes;
	}

	return writtenBytes;
}

int OpenFile::sendMsg(msgid_t id,USER const void *data1,size_t size1,
		USER const void *data2,size_t size2) {
	/* the device-messages (open, read, write, close) are always allowed and the driver can always
	 * send messages */
	if(EXPECT_FALSE(!isDeviceMsg(id & 0xFFFF) && !(flags & (VFS_MSGS | VFS_DEVICE))))
		return -EACCES;

	if(EXPECT_FALSE(!IS_CHANNEL(node->getMode())))
		return -ENOTSUP;

	return static_cast<VFSChannel*>(node)->send(flags,id,data1,size1,data2,size2);
}

ssize_t OpenFile::receiveMsg(msgid_t *id,USER void *data,size_t size,uint fflags) {
	if(EXPECT_FALSE(!IS_CHANNEL(node->getMode())))
		return -ENOTSUP;

	uint newflags = (flags & ~fflags) | fflags;
	return static_cast<VFSChannel*>(node)->receive(newflags,id,data,size);
}

int OpenFile::truncate(off_t length) {
	if(EXPECT_FALSE(!(flags & VFS_WRITE)))
		return -EACCES;

	int res = -EINVAL;
	if(devNo == VFS_DEV_NO)
		res = node->truncate(length);
	else if(IS_CHANNEL(node->getMode())) {
		VFSChannel *chan = static_cast<VFSChannel*>(node);
		res = VFSFS::truncate(chan,length);
	}
	return res;
}

int OpenFile::cancel(msgid_t mid) {
	if(EXPECT_FALSE(!IS_CHANNEL(node->getMode())))
		return -ENOTSUP;

	return static_cast<VFSChannel*>(node)->cancel(this,mid);
}

int OpenFile::delegate(pid_t pid,OpenFile *file,uint perm,int arg) {
	if(EXPECT_FALSE(!IS_CHANNEL(node->getMode())))
		return -ENOTSUP;

	return static_cast<VFSChannel*>(node)->delegate(pid,this,file,perm,arg);
}

int OpenFile::obtain(pid_t pid,int arg) {
	if(EXPECT_FALSE(!IS_CHANNEL(node->getMode())))
		return -ENOTSUP;

	return static_cast<VFSChannel*>(node)->obtain(pid,this,arg);
}

int OpenFile::bindto(tid_t tid) {
	if(EXPECT_FALSE(~flags & VFS_DEVICE))
		return -EACCES;

	if(EXPECT_TRUE(IS_CHANNEL(node->getMode()))) {
		VFSChannel *chan = static_cast<VFSChannel*>(node);
		chan->bindto(tid);
		return 0;
	}

	if(EXPECT_TRUE(IS_DEVICE(node->getMode()))) {
		VFSDevice *dev = static_cast<VFSDevice*>(node);
		dev->bindto(tid);
		return 0;
	}
	return -ENOTSUP;
}

int OpenFile::syncfs() {
	if(EXPECT_FALSE(devNo == VFS_DEV_NO))
		return -EPERM;
	if(EXPECT_FALSE(!IS_CHANNEL(node->getMode())))
		return -ENOTSUP;

	VFSChannel *chan = static_cast<VFSChannel*>(node);
	return VFSFS::syncfs(chan);
}

bool OpenFile::close() {
	LockGuard<SpinLock> g(&lock);
	return doClose();
}

bool OpenFile::doClose() {
	/* decrement references; it may be already zero if we have closed the file previously but
	 * couldn't free it because there was still a user of it. */
	if(EXPECT_TRUE(refCount > 0))
		refCount--;

	/* if there are no more references, free the file */
	if(refCount == 0) {
		/* if we have used a file-descriptor to get here, the usages are at least 1; otherwise it is
		 * 0, because it is used kernel-intern only and not passed to other "users". */
		if(usageCount <= 1) {
			node->close(this,devNo == VFS_DEV_NO ? MSG_FILE_CLOSE : MSG_FS_CLOSE);

			/* free it */
			releaseFile(this);
			return true;
		}
	}
	return false;
}

int OpenFile::getWork(OpenFile *file,uint flags) {
	if(EXPECT_FALSE(file->devNo != VFS_DEV_NO))
		return -EPERM;
	if(EXPECT_FALSE(~file->flags & VFS_DEVICE))
		return -EACCES;

	VFSDevice *dev = static_cast<VFSDevice*>(file->node);
	if(EXPECT_FALSE(!IS_DEVICE(dev->getMode())))
		return -EPERM;

	return dev->getWork(flags);
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
	for(size_t i = 0; i < gftArray.getObjCount(); i++) {
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
			os.writef("\t\tusageCount: %d\n",f->usageCount);
			os.writef("\t\tuid: %u\n",f->user.uid);
			os.writef("\t\tgid: %u\n",f->user.gid);
			os.writef("\t\tgids: ");
			for(size_t i = 0; i < f->user.groupCount; ++i) {
				os.writef("%u",f->user.gids[i]);
				if(i + 1 < f->user.groupCount)
					os.writef(",");
			}
			os.writef("\n");
			if(f->sem)
				os.writef("\t\tsem: %d\n",f->sem->sem.getValue());
			if(f->devNo == VFS_DEV_NO) {
				VFSNode *n = f->node;
				if(!n->isAlive())
					os.writef("\t\tFile: <destroyed>\n");
				else
					os.writef("\t\tFile: '%s'\n",f->getPath());
			}
			else {
				os.writef("\t\tFile: '%s'\n",f->getPath());
				os.writef("\t\tFS: '%s'\n",f->node->isAlive() ? f->node->getPath() : "<destroyed>");
			}
		}
	}
}

size_t OpenFile::getCount() {
	size_t count = 0;
	for(size_t i = 0; i < gftArray.getObjCount(); i++) {
		OpenFile *f = (OpenFile*)gftArray.getObj(i);
		if(f->flags != 0)
			count++;
	}
	return count;
}

int OpenFile::getFree(const fs::User &u,uint8_t mntperm,ushort flags,ino_t nodeNo,dev_t devNo,
		const VFSNode *n,OpenFile **f,bool clone) {
	A_UNUSED uint userFlags = VFS_READ | VFS_WRITE | VFS_MSGS | VFS_NOCHAN | VFS_NOBLOCK |
							  VFS_DEVICE | VFS_LONELY;
	size_t i;
	bool isDevice = false;
	OpenFile *e;
	/* ensure that we don't increment usages of an unused slot */
	assert(flags & (VFS_DEVICE | VFS_READ | VFS_WRITE | VFS_MSGS | VFS_NOCHAN));
	assert(!(flags & ~userFlags));

	if(devNo == VFS_DEV_NO)
		isDevice = IS_CHANNEL(n->getMode());
	/* doesn't work for channels and pipes */
	if(EXPECT_FALSE(isDevice && (flags & VFS_LONELY)))
		return -EINVAL;

	{
		LockGuard<SpinLock> g(&gftLock);
		/* devices and files can't be used exclusively */
		if(!clone && !isDevice) {
			/* check if somebody has this file currently exclusively */
			e = exclList;
			while(e) {
				assert(e->flags != 0);
				/* same file? */
				if(e->devNo == devNo && e->nodeNo == nodeNo) {
					assert(e->flags & VFS_LONELY);
					return -EBUSY;
				}
				e = e->next;
			}

			/* if we want to have it exclusively, check if it is already open */
			if(flags & VFS_LONELY) {
				e = usedList;
				while(e) {
					assert(e->flags != 0);
					assert(~e->flags & VFS_LONELY);
					/* same file? */
					if(e->devNo == devNo && e->nodeNo == nodeNo)
						return -EBUSY;
					e = e->next;
				}
			}
		}

		/* if there is no free slot anymore, extend our dyn-array */
		if(EXPECT_FALSE(gftFreeList == NULL)) {
			size_t j;
			i = gftArray.getObjCount();
			if(!gftArray.extend())
				return -ENFILE;
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

		/* insert in corresponding list */
		if(flags & VFS_LONELY) {
			e->next = exclList;
			exclList = e;
		}
		else {
			e->next = usedList;
			usedList = e;
		}
	}

	/* count references of virtual nodes */
	e->node = const_cast<VFSNode*>(n);
	e->node->ref();
	e->user = u;
	e->mntperm = mntperm;
	e->flags = flags;
	e->refCount = 1;
	e->usageCount = 0;
	e->position = 0;
	e->devNo = devNo;
	e->nodeNo = nodeNo;
	e->path = NULL;
	e->sem = NULL;
	return 0;
}

void OpenFile::releaseFile(OpenFile *file) {
	Cache::free(file->path);
	if(file->sem) {
		bool last;
		{
			LockGuard<SpinLock> g(&semLock);
			last = --file->sem->refs == 0;
			if(last)
				sems.remove(file->sem);
		}
		if(last)
			delete file->sem;
	}

	LockGuard<SpinLock> g(&gftLock);
	assert(file->flags != 0);
	OpenFile *e = (file->flags & VFS_LONELY) ? exclList : usedList;
	OpenFile *p = NULL;
	while(e) {
		if(e == file)
			break;
		p = e;
		e = e->next;
	}
	assert(e);
	if(p)
		p->next = e->next;
	else if(file->flags & VFS_LONELY)
		exclList = e->next;
	else
		usedList = e->next;
	file->flags = 0;
	file->next = gftFreeList;
	gftFreeList = file;
}
