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

#include <fs/permissions.h>
#include <mem/cache.h>
#include <mem/dynarray.h>
#include <mem/pagedir.h>
#include <sys/messages.h>
#include <task/filedesc.h>
#include <task/groups.h>
#include <task/proc.h>
#include <task/timer.h>
#include <usergroup/usergroup.h>
#include <vfs/channel.h>
#include <vfs/device.h>
#include <vfs/dir.h>
#include <vfs/file.h>
#include <vfs/fs.h>
#include <vfs/info.h>
#include <vfs/node.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <assert.h>
#include <common.h>
#include <cppsupport.h>
#include <errno.h>
#include <ostringstream.h>
#include <string.h>
#include <util.h>
#include <video.h>

VFSNode *VFS::pidsNode;
VFSNode *VFS::procsNode;
VFSNode *VFS::devNode;
VFSNode *VFS::tmpNode;
VFSNode *VFS::mountsNode;

void VFS::init() {
	VFSNode *root,*sys;

	const fs::User kern = fs::User::kernel();

	/*
	 *  /
	 *   |- sys
	 *   |   |- boot
	 *   |   |- dev
	 *   |   |- irq
	 *   |   |- pid
	 *   |       \- self
	 *   |   |- proc
	 *   |   \- mount
	 *   |- dev
	 *   \- tmp
	 */
	root = createObj<VFSDir>(kern,nullptr,(char*)"",DIR_DEF_MODE);
	sys = createObj<VFSDir>(kern,root,(char*)"sys",S_IFDIR | 0775);
	sys->chown(kern,ROOT_UID,GROUP_DRIVER);
	VFSNode::release(createObj<VFSDir>(kern,sys,(char*)"boot",DIR_DEF_MODE));
	procsNode = createObj<VFSDir>(kern,sys,(char*)"proc",DIR_DEF_MODE);
	pidsNode = createObj<VFSDir>(kern,sys,(char*)"pid",DIR_DEF_MODE);
	VFSNode::release(createObj<VFSInfo::SelfLinkFile>(kern,pidsNode,(char*)"self",LNK_DEF_MODE));
	VFSNode *dev = createObj<VFSDir>(kern,sys,(char*)"dev",S_IFDIR | 0775);
	dev->chown(kern,ROOT_UID,GROUP_DRIVER);
	VFSNode::release(dev);
	VFSNode::release(createObj<VFSDir>(kern,sys,(char*)"irq",DIR_DEF_MODE));
	mountsNode = createObj<VFSDir>(kern,sys,(char*)"mount",DIR_DEF_MODE);
	VFSNode::release(mountsNode);
	devNode = createObj<VFSDir>(kern,root,(char*)"dev",DIR_DEF_MODE);
	/* the user should be able to create devices as well */
	/* TODO: maybe we should organize that differently */
	devNode->chmod(kern,0777);
	VFSNode::release(devNode);
	tmpNode = createObj<VFSDir>(kern,root,(char*)"tmp",S_IFDIR | S_ISSTICKY | 0777);
	VFSNode::release(tmpNode);
	VFSNode::release(pidsNode);
	VFSNode::release(procsNode);
	VFSNode::release(sys);
	VFSNode::release(root);

	VFSInfo::init(sys);
}

void VFS::mountAll(Proc *p) {
	const uint rwx = VFS_READ | VFS_WRITE | VFS_EXEC;
	const fs::User kern = fs::User::kernel();
	OpenFile *devFile,*sysFile,*tmpFile;

	if(openFile(kern,rwx,rwx,devNode,devNode->getNo(),VFS_DEV_NO,&devFile) < 0)
		Util::panic("Unable to open /dev");
	if(p->getMS()->mount("/dev",devFile) < 0)
		Util::panic("Unable to mount /dev");

	VFSNode *sys = procsNode->getParent();
	if(openFile(kern,rwx,rwx,sys,sys->getNo(),VFS_DEV_NO,&sysFile) < 0)
		Util::panic("Unable to open /sys");
	if(p->getMS()->mount("/sys",sysFile) < 0)
		Util::panic("Unable to mount /sys");

	if(openFile(kern,rwx,rwx,tmpNode,tmpNode->getNo(),VFS_DEV_NO,&tmpFile) < 0)
		Util::panic("Unable to open /tmp");
	if(p->getMS()->mount("/tmp",tmpFile) < 0)
		Util::panic("Unable to mount /tmp");
}

int VFS::cloneMS(Proc *p,const char *name) {
	MntSpace *src = p->getMS();
	size_t len = strlen(name);

	// check if the file exists
	if(src->getNode()->findInDir(name,len))
		return -EEXIST;

	// create name copy
	char *copy = (char*)Cache::alloc(len + 1);
	if(!copy)
		return -ENOMEM;
	strcpy(copy,name);

	// create new mountspace
	const fs::User user(p->getUid(),p->getGid());
	MntSpace *ms = src->clone(user,copy);
	if(ms == NULL) {
		Cache::free(copy);
		return -ENOMEM;
	}

	// join mountspace
	ms->join(p);
	return 0;
}

int VFS::joinMS(Proc *p,MntSpace *src) {
	src->join(p);
	return 0;
}

int VFS::hasAccess(const fs::User &u,const VFSNode *n,ushort flags) {
	if(!n->isAlive())
		return -ENOENT;

	uint perm = 0;
	if(flags & VFS_READ)
		perm |= MODE_READ;
	if(flags & VFS_WRITE)
		perm |= MODE_WRITE;
	if(flags & VFS_EXEC)
		perm |= MODE_EXEC;
	return fs::Permissions::canAccess(&u,n->getMode(),n->getUid(),n->getGid(),perm);
}

int VFS::hasAccess(pid_t pid,const VFSNode *n,ushort flags) {
	const Proc *p = Proc::getByPid(pid);
	fs::User user(p->getUid(),p->getGid());
	user.groupCount = Groups::get(pid,user.gids,fs::MAX_GROUPS);

	return hasAccess(user,n,flags);
}

int VFS::hasAccess(pid_t pid,mode_t mode,uid_t uid,gid_t gid,ushort flags) {
	/* kernel is allmighty :P */
	if(pid == KERNEL_PID)
		return 0;

	const Proc *p = Proc::getByPid(pid);
	if(p == NULL)
		return -ESRCH;

	int mask;
	if(p->getUid() == ROOT_UID) {
		/* root has exec-permission if at least one has exec-permission */
		if(flags & VFS_EXEC)
			return (mode & MODE_EXEC) ? 0 : -EACCES;
		/* root can read and write in all cases */
		return 0;
	}

	/* determine mask */
	if(uid == p->getUid())
		mask = mode & S_IRWXU;
	else if(gid == p->getGid() || Groups::contains(pid,gid) == 1)
		mask = mode & S_IRWXG;
	else
		mask = mode & S_IRWXO;

	/* check access */
	if((flags & VFS_READ) && !(mask & MODE_READ))
		return -EACCES;
	if((flags & VFS_WRITE) && !(mask & MODE_WRITE))
		return -EACCES;
	if((flags & VFS_EXEC) && !(mask & MODE_EXEC))
		return -EACCES;
	return 0;
}

int VFS::openPath(pid_t pid,ushort flags,mode_t mode,const char *path,ssize_t *sympos,OpenFile **file) {
	OpenFile *fsFile;
	const char *begin;
	int err;

	Proc *p = Proc::getByPid(pid);
	ino_t root = p->getMS()->request(path,&begin,&fsFile);
	if(root < 0)
		return root;

	fs::User user(p->getUid(),p->getGid());
	user.groupCount = Groups::get(pid,user.gids,fs::MAX_GROUPS);

	/* check if the file to the mounted filesystem has the permissions we are requesting */
	const uint rwx = VFS_READ | VFS_WRITE | VFS_EXEC;
	if(~(fsFile->getFlags() &  rwx) & (flags & rwx)) {
		err = -EACCES;
		goto errorMnt;
	}

	/* if it's in the virtual fs, it's a directory, not a channel */
	VFSNode *node;
	msgid_t openmsg;
	if(!IS_CHANNEL(fsFile->getNode()->getMode())) {
		if(root == 0)
			node = fsFile->getNode();
		else
			node = VFSNode::get(root);
		VFSNode::RequestResult rres;
		err = VFSNode::request(user,begin,node,&rres,flags,mode);
		if(err < 0)
			goto errorMnt;
		node = rres.node;
		if(rres.sympos != -1 && sympos) {
			flags = VFS_READ;
			*sympos = (begin - path) + rres.sympos;
			sympos = NULL;
		}
		if((err = hasAccess(user,node,flags)) < 0)
			goto error;

		begin = rres.end;
		openmsg = MSG_FILE_OPEN;
	}
	/* otherwise use the device-node of the fs */
	else {
		node = fsFile->getNode();
		node = VFSNode::request(node->getParent()->getNo());

		openmsg = MSG_FS_OPEN;
	}

	/* if its a device, create the channel-node, by default. If VFS_NOCHAN is given, don't do that
	 * unless it's a file in a userspace FS (openmsg == MSG_FS_OPEN). */
	if(IS_DEVICE(node->getMode()) && (openmsg == MSG_FS_OPEN || !(flags & VFS_NOCHAN))) {
		VFSNode *child = createObj<VFSChannel>(user,node);
		VFSNode::release(node);
		if(child == NULL) {
			err = -ENOMEM;
			goto errorMnt;
		}
		node = child;
	}

	/* give the node a chance to react on it */
	err = node->open(user,begin,sympos,root,flags,openmsg,mode);
	if(err < 0)
		goto error;
	/* symlink found? */
	if(sympos && *sympos != -1) {
		*sympos += begin - path;
		flags = VFS_READ;
	}

	/* open file */
	if(openmsg == MSG_FS_OPEN)
		err = openFile(user,fsFile->getFlags(),flags,node,err,fsFile->getNodeNo(),file);
	else
		err = openFile(user,fsFile->getFlags(),flags,node,node->getNo(),VFS_DEV_NO,file);
	if(err < 0)
		goto error;

	/* store the path for debugging purposes */
	if(openmsg == MSG_FS_OPEN)
		(*file)->setPath(strdup(path));
	VFSNode::release(node);

	/* append? */
	if(flags & VFS_APPEND) {
		err = (*file)->seek(0,SEEK_END);
		if(err < 0) {
			(*file)->close();
			return err;
		}
	}
	MntSpace::release(fsFile);
	return 0;

error:
	VFSNode::release(node);
errorMnt:
	MntSpace::release(fsFile);
	return err;
}

int VFS::openFile(const fs::User &u,uint8_t mntperms,ushort flags,const VFSNode *node,ino_t nodeNo,
				  dev_t devNo,OpenFile **file) {
	int err;

	/* cleanup flags */
	flags &= VFS_READ | VFS_WRITE | VFS_MSGS | VFS_NOCHAN | VFS_NOBLOCK | VFS_DEVICE | VFS_LONELY;

	if(EXPECT_FALSE(devNo == VFS_DEV_NO && (err = hasAccess(u,node,flags)) < 0))
		return err;

	/* determine free file */
	return OpenFile::getFree(u,mntperms,flags,nodeNo,devNo,node,file,false);
}

int VFS::openFileDesc(pid_t pid,uint8_t mntperms,ushort flags,const VFSNode *node,ino_t nodeNo,dev_t devNo) {
	OpenFile *file;
	int res;

	Proc *p = Proc::getRef(pid);
	if(!p)
		return -EDESTROYED;

	fs::User user(p->getUid(),p->getGid());
	user.groupCount = Groups::get(pid,user.gids,fs::MAX_GROUPS);

	/* no permission check here; and the flags are already valid */
	res = OpenFile::getFree(user,mntperms,flags,nodeNo,devNo,node,&file,true);
	if(res < 0)
		goto errorProc;

	res = FileDesc::assoc(p,file);
	if(res < 0)
		goto errorAssoc;
	Proc::relRef(p);
	return res;

errorAssoc:
	file->close();
errorProc:
	Proc::relRef(p);
	return res;
}

void VFS::closeFileDesc(pid_t pid,int fd) {
	Proc *p = Proc::getRef(pid);
	if(p) {
		/* the file might have been closed already */
		OpenFile *file = FileDesc::unassoc(p,fd);
		if(file)
			file->close();
		Proc::relRef(p);
	}
}

ino_t VFS::createProcess(pid_t pid) {
	int res = -ENOMEM;
	Proc *p = Proc::getByPid(pid);
	assert(p != NULL);

	/* build name */
	char *name = (char*)Cache::alloc(12);
	if(name == NULL)
		return -ENOMEM;
	itoa(name,12,pid);

	VFSNode *proc,*dir,*nn;
	ino_t threadsIno;
	const fs::User kern = fs::User::kernel();

	/* put it in parent's proc directory */
	if(p->getParentPid() != p->getPid()) {
		Proc *pp = Proc::getRef(p->getParentPid());
		if(!pp) {
			res = -ENOENT;
			goto errorName;
		}
		proc = VFSNode::get(pp->getThreadsDir())->getParent();
		Proc::relRef(pp);
	}
	else
		proc = procsNode;

	/* create dir */
	dir = createObj<VFSDir>(kern,proc,name,MODE_TYPE_PROC | DIR_DEF_MODE);
	if(dir == NULL)
		goto errorName;
	/* change owner to user+group of process */
	if(dir->chown(kern,p->getUid(),p->getGid()) < 0)
		goto errorDir;

	/* create process-info-node */
	nn = createObj<VFSInfo::ProcFile>(kern,dir);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create virt-mem-info-node */
	nn = createObj<VFSInfo::VirtMemFile>(kern,dir);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create regions-info-node */
	nn = createObj<VFSInfo::RegionsFile>(kern,dir);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create maps-info-node */
	nn = createObj<VFSInfo::MapFile>(kern,dir);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create mountspace-link-node */
	nn = createObj<VFSInfo::MSLinkFile>(kern,dir,(char*)"ms",S_IFLNK | LNK_DEF_MODE);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create shm-dir */
	nn = createObj<VFSDir>(kern,dir,(char*)"shm",S_IFDIR | 0777);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create threads-dir */
	nn = createObj<VFSDir>(kern,dir,(char*)"threads",DIR_DEF_MODE);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* note that it is ok to use the number and don't care about references since the kernel owns
	 * the nodes and thus, nobody can destroy them */
	threadsIno = nn->getNo();

	/* create link in /sys/pid */
	{
		char *namecpy = strdup(name);
		if(!namecpy) {
			Cache::free(name);
			goto errorDir;
		}
		nn = createObj<VFSInfo::PidLinkFile>(kern,pidsNode,namecpy,S_IFLNK | LNK_DEF_MODE);
		if(nn == NULL) {
			Cache::free(namecpy);
			goto errorDir;
		}
		VFSNode::release(nn);
	}

	VFSNode::release(dir);

	/* note that it is ok to use the number and don't care about references since the kernel owns
	 * the nodes and thus, nobody can destroy them */
	return threadsIno;

errorDir:
	VFSNode::release(dir);
	VFSNode::release(dir);
	/* name is free'd by dir */
	return res;
errorName:
	Cache::free(name);
	return res;
}

void VFS::chownProcess(pid_t pid,uid_t uid,gid_t gid) {
	const Proc *p = Proc::getByPid(pid);
	VFSNode *node = VFSNode::get(p->getThreadsDir());
	node->getParent()->chown(fs::User::kernel(),uid,gid);
}

void VFS::moveProcess(pid_t pid,pid_t oldppid,pid_t newppid) {
	char name[12];
	itoa(name,sizeof(name),pid);

	Proc *opp = Proc::getRef(oldppid);
	if(!opp)
		return;

	VFSNode *src = VFSNode::get(opp->getThreadsDir())->getParent();

	Proc *npp = Proc::getRef(newppid);
	if(!npp) {
		Proc::relRef(opp);
		return;
	}
	VFSNode *dst = VFSNode::get(npp->getThreadsDir())->getParent();

	src->rename(fs::User::kernel(),name,dst,name);
	Proc::relRef(npp);
	Proc::relRef(opp);
}

void VFS::removeProcess(pid_t pid) {
	/* remove from /sys/proc */
	const Proc *p = Proc::getByPid(pid);
	VFSNode *node = VFSNode::get(p->getThreadsDir());
	node->getParent()->destroy();

	char name[12];
	itoa(name,sizeof(name),pid);
	pidsNode->unlink(fs::User::kernel(),name);
}

ino_t VFS::createThread(tid_t tid) {
	VFSNode *n,*dir;
	const Thread *t = Thread::getById(tid);

	/* build name */
	char *name = (char*)Cache::alloc(12);
	if(name == NULL)
		return -ENOMEM;
	itoa(name,12,tid);

	const fs::User kern = fs::User::kernel();

	/* create dir */
	n = VFSNode::get(t->getProc()->getThreadsDir());
	dir = createObj<VFSDir>(kern,n,name,DIR_DEF_MODE);
	if(dir == NULL) {
		Cache::free(name);
		goto errorDir;
	}

	/* create info-node */
	n = createObj<VFSInfo::ThreadFile>(kern,dir);
	if(n == NULL)
		goto errorInfo;
	VFSNode::release(n);

	/* create trace-node */
	n = createObj<VFSInfo::TraceFile>(kern,dir);
	if(n == NULL)
		goto errorInfo;
	VFSNode::release(n);
	VFSNode::release(dir);
	return dir->getNo();

errorInfo:
	VFSNode::release(dir);
	VFSNode::release(dir);
	/* name is free'd by dir */
errorDir:
	return -ENOMEM;
}

void VFS::removeThread(tid_t tid) {
	Thread *t = Thread::getById(tid);
	ino_t nodeNo = t->getThreadDir();
	if(nodeNo != -1) {
		VFSNode *n = VFSNode::get(nodeNo);
		n->destroy();
	}
}

void VFS::printMsgs(OStream &os) {
	bool isValid;
	const VFSNode *drv = devNode->openDir(true,&isValid);
	if(isValid) {
		while(drv != NULL) {
			if(IS_DEVICE(drv->getMode()))
				drv->print(os);
			drv = drv->next;
		}
	}
	devNode->closeDir(true);
}
