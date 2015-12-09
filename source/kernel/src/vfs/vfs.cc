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
#include <mem/dynarray.h>
#include <mem/pagedir.h>
#include <sys/messages.h>
#include <task/groups.h>
#include <task/proc.h>
#include <task/timer.h>
#include <vfs/channel.h>
#include <vfs/device.h>
#include <vfs/dir.h>
#include <vfs/file.h>
#include <vfs/fs.h>
#include <vfs/info.h>
#include <vfs/link.h>
#include <vfs/node.h>
#include <vfs/openfile.h>
#include <vfs/selflink.h>
#include <vfs/vfs.h>
#include <assert.h>
#include <common.h>
#include <cppsupport.h>
#include <errno.h>
#include <string.h>
#include <util.h>
#include <video.h>

VFSNode *VFS::procsNode;
VFSNode *VFS::devNode;
VFSNode *VFS::msNode;
SpinLock waitLock;

void VFS::init() {
	VFSNode *root,*sys;

	/*
	 *  /
	 *   |- sys
	 *   |   |- boot
	 *   |   |- shm
	 *   |   |- devices
	 *   |   |- fs
	 *   |   |- ms
	 *   |   \- proc
	 *   |       \- self
	 *   \- dev
	 */
	root = createObj<VFSDir>(KERNEL_PID,nullptr,(char*)"",DIR_DEF_MODE);
	sys = createObj<VFSDir>(KERNEL_PID,root,(char*)"sys",DIR_DEF_MODE);
	VFSNode::release(createObj<VFSDir>(KERNEL_PID,sys,(char*)"boot",DIR_DEF_MODE));
	VFSNode *node = createObj<VFSDir>(KERNEL_PID,sys,(char*)"shm",DIR_DEF_MODE);
	/* the user should be able to create shms as well */
	node->chmod(KERNEL_PID,0777);
	VFSNode::release(node);
	procsNode = createObj<VFSDir>(KERNEL_PID,sys,(char*)"proc",DIR_DEF_MODE);
	VFSNode::release(createObj<VFSSelfLink>(KERNEL_PID,procsNode,(char*)"self"));
	VFSNode::release(createObj<VFSDir>(KERNEL_PID,sys,(char*)"devices",DIR_DEF_MODE));
	VFSNode::release(createObj<VFSDir>(KERNEL_PID,sys,(char*)"fs",DIR_DEF_MODE));
	msNode = createObj<VFSDir>(KERNEL_PID,sys,(char*)"ms",DIR_DEF_MODE);
	VFSNode::release(msNode);
	devNode = createObj<VFSDir>(KERNEL_PID,root,(char*)"dev",DIR_DEF_MODE);
	/* the user should be able to create devices as well */
	/* TODO: maybe we should organize that differently */
	devNode->chmod(KERNEL_PID,0777);
	VFSNode::release(devNode);
	VFSNode::release(procsNode);
	VFSNode::release(sys);
	VFSNode::release(root);

	VFSInfo::init(sys);
}

void VFS::mountAll(Proc *p) {
	if(p->getMS()->mount(p,"/dev",reinterpret_cast<OpenFile*>(devNode)) < 0)
		Util::panic("Unable to mount /dev");
	if(p->getMS()->mount(p,"/sys",reinterpret_cast<OpenFile*>(procsNode->getParent())) < 0)
		Util::panic("Unable to mount /dev");
}

int VFS::cloneMS(Proc *p,const VFSMS *src,const char *name) {
	size_t len = strlen(name);
	char *copy = (char*)Cache::alloc(len + 1);
	if(!copy)
		return -ENOMEM;
	strcpy(copy,name);

	p->msnode = createObj<VFSMS>(p->getPid(),*src,msNode,copy,0644);
	if(p->msnode == NULL) {
		Cache::free(copy);
		return -ENOMEM;
	}
	return 0;
}

int VFS::hasAccess(pid_t pid,const VFSNode *n,ushort flags) {
	const Proc *p;
	if(!n->isAlive())
		return -ENOENT;
	/* kernel is allmighty :P */
	if(pid == KERNEL_PID)
		return 0;

	p = Proc::getByPid(pid);
	if(p == NULL)
		return -ESRCH;
	/* root is (nearly) allmighty as well */
	if(p->getEUid() == ROOT_UID) {
		/* root has exec-permission if at least one has exec-permission */
		if(flags & VFS_EXEC)
			return (n->getMode() & MODE_EXEC) ? 0 : -EACCES;
		return 0;
	}

	/* determine mask */
	uint mode;
	if(p->getEUid() == n->getUid())
		mode = n->getMode() & S_IRWXU;
	else if(p->getEGid() == n->getGid() || Groups::contains(p->getPid(),n->getGid()))
		mode = n->getMode() & S_IRWXG;
	else
		mode = n->getMode() & S_IRWXO;

	/* check access */
	if((flags & VFS_READ) && !(mode & MODE_READ))
		return -EACCES;
	if((flags & VFS_WRITE) && !(mode & MODE_WRITE))
		return -EACCES;
	if((flags & VFS_EXEC) && !(mode & MODE_EXEC))
		return -EACCES;
	return 0;
}

int VFS::request(pid_t pid,const char *path,ushort flags,mode_t mode,const char **begin,OpenFile **res) {
	Proc *p = Proc::getByPid(pid);
	int err = p->getMS()->request(path,begin,res);
	if(err < 0)
		return err;

	/* if it's in the virtual fs, it is a VFSNode, not an OpenFile */
	if(IS_NODE(*res) && !(flags & VFS_NONODERES)) {
		VFSNode *node = reinterpret_cast<VFSNode*>(*res);
		const char *vpath = *begin;
		err = VFSNode::request(vpath,begin,&node,NULL,flags,mode);
		*res = reinterpret_cast<OpenFile*>(node);
	}
	return err;
}

int VFS::openPath(pid_t pid,ushort flags,mode_t mode,const char *path,OpenFile **file) {
	OpenFile *fsFile;
	const char *begin;
	int err = request(pid,path,flags,mode,&begin,&fsFile);
	if(err < 0)
		return err;

	/* if it's in the virtual fs, it is a VFSNode, not an OpenFile */
	VFSNode *node;
	msgid_t openmsg;
	if(IS_NODE(fsFile)) {
		node = reinterpret_cast<VFSNode*>(fsFile);
		openmsg = MSG_FILE_OPEN;
	}
	/* otherwise use the device-node of the fs */
	else {
		node = fsFile->getNode();
		node = VFSNode::request(node->getParent()->getNo());
		openmsg = MSG_FS_OPEN;
	}

	ino_t nodeNo = node->getNo();
	/* if its a device, create the channel-node, by default. If VFS_NOCHAN is given, don't do that
	 * unless it's a file in a userspace FS (openmsg == MSG_FS_OPEN). */
	if(IS_DEVICE(node->getMode()) && (openmsg == MSG_FS_OPEN || !(flags & VFS_NOCHAN))) {
		VFSNode *child;
		/* check if we can access the device */
		if((err = hasAccess(pid,node,flags)) < 0)
			goto error;
		child = createObj<VFSChannel>(pid,node);
		VFSNode::release(node);
		if(child == NULL) {
			err = -ENOMEM;
			goto errorMnt;
		}
		node = child;
	}

	/* give the node a chance to react on it */
	err = node->open(pid,begin,flags,openmsg,mode);
	if(err < 0)
		goto error;

	/* open file */
	if(IS_NODE(fsFile))
		err = openFile(pid,flags,node,nodeNo,VFS_DEV_NO,file);
	else
		err = openFile(pid,flags,node,err,fsFile->getNodeNo(),file);
	if(err < 0)
		goto error;

	/* store the path for debugging purposes */
	if(!IS_NODE(fsFile))
		(*file)->setPath(strdup(path));
	VFSNode::release(node);

	/* append? */
	if(flags & VFS_APPEND) {
		err = (*file)->seek(pid,0,SEEK_END);
		if(err < 0) {
			(*file)->close(pid);
			return err;
		}
	}
	VFSMS::release(fsFile);
	return 0;

error:
	VFSNode::release(node);
errorMnt:
	VFSMS::release(fsFile);
	return err;
}

int VFS::openFile(pid_t pid,ushort flags,const VFSNode *node,ino_t nodeNo,dev_t devNo,
                  OpenFile **file) {
	int err;

	/* cleanup flags */
	flags &= VFS_READ | VFS_WRITE | VFS_MSGS | VFS_NOCHAN | VFS_NOBLOCK | VFS_DEVICE | VFS_LONELY;

	if(EXPECT_FALSE(devNo == VFS_DEV_NO && (err = hasAccess(pid,node,flags)) < 0))
		return err;

	/* determine free file */
	return OpenFile::getFree(pid,flags,nodeNo,devNo,node,file);
}

int VFS::creatsibl(pid_t pid,OpenFile *file,int arg,OpenFile **sibl) {
	VFSNode *node = file->getNode();
	if(!IS_CHANNEL(node->getMode()))
		return -ENOTSUP;

	/* create sibling */
	VFSChannel *siblChan = createObj<VFSChannel>(pid,node->getParent());
	if(siblChan == NULL)
		return -ENOMEM;

	/* open new file for the sibling */
	int res = openFile(pid,file->getFlags(),siblChan,siblChan->getNo(),VFS_DEV_NO,sibl);
	if(res < 0) {
		VFSNode::release(siblChan);
		VFSNode::release(siblChan);
		return res;
	}

	/* the file holds a reference now */
	VFSNode::release(siblChan);

	/* talk to the driver */
	res = file->creatsibl(pid,*sibl,arg);
	if(res < 0) {
		(*sibl)->close(pid);
		return res;
	}
	return res;
}

ino_t VFS::createProcess(pid_t pid,VFSNode *ms) {
	VFSNode *proc = procsNode,*dir,*nn;
	int res = -ENOMEM;

	/* build name */
	char *name = (char*)Cache::alloc(12);
	if(name == NULL)
		return -ENOMEM;
	itoa(name,12,pid);

	/* create dir */
	dir = createObj<VFSDir>(KERNEL_PID,proc,name,DIR_DEF_MODE);
	if(dir == NULL)
		goto errorName;

	/* create process-info-node */
	nn = createObj<VFSInfo::ProcFile>(KERNEL_PID,dir);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create virt-mem-info-node */
	nn = createObj<VFSInfo::VirtMemFile>(KERNEL_PID,dir);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create regions-info-node */
	nn = createObj<VFSInfo::RegionsFile>(KERNEL_PID,dir);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create maps-info-node */
	nn = createObj<VFSInfo::MapFile>(KERNEL_PID,dir);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create mountspace-link-node */
	nn = createObj<VFSLink>(KERNEL_PID,dir,(char*)"ms",ms);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create shm-dir */
	nn = createObj<VFSDir>(KERNEL_PID,dir,(char*)"shm",0777);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create threads-dir */
	nn = createObj<VFSDir>(KERNEL_PID,dir,(char*)"threads",DIR_DEF_MODE);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	VFSNode::release(dir);
	/* note that it is ok to use the number and don't care about references since the kernel owns
	 * the nodes and thus, nobody can destroy them */
	return nn->getNo();

errorDir:
	VFSNode::release(dir);
	VFSNode::release(dir);
	/* name is free'd by dir */
	return res;
errorName:
	Cache::free(name);
	return res;
}

void VFS::removeProcess(pid_t pid) {
	/* remove from /sys/proc */
	const Proc *p = Proc::getByPid(pid);
	VFSNode *node = VFSNode::get(p->getThreadsDir());
	node->getParent()->destroy();
}

ino_t VFS::createThread(tid_t tid) {
	VFSNode *n,*dir;
	const Thread *t = Thread::getById(tid);

	/* build name */
	char *name = (char*)Cache::alloc(12);
	if(name == NULL)
		return -ENOMEM;
	itoa(name,12,tid);

	/* create dir */
	n = VFSNode::get(t->getProc()->getThreadsDir());
	dir = createObj<VFSDir>(KERNEL_PID,n,name,DIR_DEF_MODE);
	if(dir == NULL) {
		Cache::free(name);
		goto errorDir;
	}

	/* create info-node */
	n = createObj<VFSInfo::ThreadFile>(KERNEL_PID,dir);
	if(n == NULL)
		goto errorInfo;
	VFSNode::release(n);

	/* create trace-node */
	n = createObj<VFSInfo::TraceFile>(KERNEL_PID,dir);
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
