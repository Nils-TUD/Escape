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
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/fs.h>
#include <sys/vfs/info.h>
#include <sys/vfs/file.h>
#include <sys/vfs/dir.h>
#include <sys/vfs/link.h>
#include <sys/vfs/selflink.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/pipe.h>
#include <sys/vfs/device.h>
#include <sys/vfs/openfile.h>
#include <sys/task/proc.h>
#include <sys/task/groups.h>
#include <sys/task/timer.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/cache.h>
#include <sys/mem/dynarray.h>
#include <sys/util.h>
#include <sys/video.h>
#include <sys/cppsupport.h>
#include <esc/messages.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

VFSNode *VFS::procsNode;
VFSNode *VFS::devNode;
SpinLock waitLock;

void VFS::init() {
	VFSNode *root,*sys;

	/*
	 *  /
	 *   |- system
	 *   |   |- pipes
	 *   |   |- mbmods
	 *   |   |- shm
	 *   |   |- devices
	 *   |   |- fs
	 *   |   \- processes
	 *   |       \- self
	 *   \- dev
	 */
	root = createObj<VFSDir>(KERNEL_PID,nullptr,(char*)"",DIR_DEF_MODE);
	sys = createObj<VFSDir>(KERNEL_PID,root,(char*)"system",DIR_DEF_MODE);
	VFSNode::release(createObj<VFSDir>(KERNEL_PID,sys,(char*)"pipes",DIR_DEF_MODE));
	VFSNode::release(createObj<VFSDir>(KERNEL_PID,sys,(char*)"mbmods",DIR_DEF_MODE));
	VFSNode *node = createObj<VFSDir>(KERNEL_PID,sys,(char*)"shm",DIR_DEF_MODE);
	/* the user should be able to create shms as well */
	node->chmod(KERNEL_PID,0777);
	VFSNode::release(node);
	procsNode = createObj<VFSDir>(KERNEL_PID,sys,(char*)"processes",DIR_DEF_MODE);
	VFSNode::release(createObj<VFSSelfLink>(KERNEL_PID,procsNode,(char*)"self"));
	VFSNode::release(createObj<VFSDir>(KERNEL_PID,sys,(char*)"devices",DIR_DEF_MODE));
	VFSNode::release(createObj<VFSDir>(KERNEL_PID,sys,(char*)"fs",DIR_DEF_MODE));
	devNode = createObj<VFSDir>(KERNEL_PID,root,(char*)"dev",DIR_DEF_MODE);
	VFSNode::release(devNode);
	VFSNode::release(procsNode);
	VFSNode::release(sys);
	VFSNode::release(root);

	VFSInfo::init(sys);
}

void VFS::mountAll(Proc *p) {
	if(MountSpace::mount(p,"/dev",reinterpret_cast<OpenFile*>(devNode)) < 0)
		Util::panic("Unable to mount /dev");
	if(MountSpace::mount(p,"/system",reinterpret_cast<OpenFile*>(procsNode->getParent())) < 0)
		Util::panic("Unable to mount /dev");
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
	int err = MountSpace::request(p,path,begin,res);
	if(err < 0)
		return err;

	/* if it's in the virtual fs, it is a VFSNode, not an OpenFile */
	if(IS_NODE(*res) && !(flags & VFS_NONODERES)) {
		VFSNode *node = reinterpret_cast<VFSNode*>(*res);
		err = VFSNode::request(*begin,&node,NULL,flags,mode);
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

	/* if its a device, create the channel-node */
	inode_t nodeNo = node->getNo();
	if(IS_DEVICE(node->getMode())) {
		VFSNode *child;
		/* check if we can access the device */
		if((err = hasAccess(pid,node,flags)) < 0) {
			VFSNode::release(node);
			return err;
		}
		child = createObj<VFSChannel>(pid,node);
		VFSNode::release(node);
		if(child == NULL)
			return -ENOMEM;
		node = child;
	}

	/* give the node a chance to react on it */
	err = node->open(pid,begin,flags,openmsg);
	if(err < 0) {
		VFSNode::release(node);
		if(!IS_NODE(fsFile))
			MountSpace::release(fsFile);
		return err;
	}

	/* open file */
	if(IS_NODE(fsFile))
		err = openFile(pid,flags,node,nodeNo,VFS_DEV_NO,file);
	else
		err = openFile(pid,flags,node,err,fsFile->getNodeNo(),file);
	if(err < 0) {
		VFSNode::release(node);
		if(!IS_NODE(fsFile))
			MountSpace::release(fsFile);
		return err;
	}

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
	return 0;
}

int VFS::openPipe(pid_t pid,OpenFile **readFile,OpenFile **writeFile) {
	/* resolve pipe-path */
	VFSNode *node = NULL;
	int err = VFSNode::request("/system/pipes",&node,NULL,VFS_READ,0);
	if(err < 0)
		return err;

	/* create pipe */
	VFSNode *pipeNode = createObj<VFSPipe>(pid,node);
	VFSNode::release(node);
	if(pipeNode == NULL)
		return -ENOMEM;

	/* open file for reading */
	err = openFile(pid,VFS_READ,pipeNode,pipeNode->getNo(),VFS_DEV_NO,readFile);
	if(err < 0) {
		VFSNode::release(pipeNode);
		VFSNode::release(pipeNode);
		return err;
	}

	/* open file for writing */
	err = openFile(pid,VFS_WRITE,pipeNode,pipeNode->getNo(),VFS_DEV_NO,writeFile);
	if(err < 0) {
		VFSNode::release(pipeNode);
		/* closeFile removes the pipenode, too */
		(*readFile)->close(pid);
		return err;
	}
	VFSNode::release(pipeNode);
	return 0;
}

int VFS::openFile(pid_t pid,ushort flags,const VFSNode *node,inode_t nodeNo,dev_t devNo,
                  OpenFile **file) {
	int err;

	/* cleanup flags */
	flags &= VFS_READ | VFS_WRITE | VFS_MSGS | VFS_NOBLOCK | VFS_DEVICE | VFS_EXCLUSIVE;

	if(EXPECT_FALSE(devNo == VFS_DEV_NO && (err = hasAccess(pid,node,flags)) < 0))
		return err;

	/* determine free file */
	return OpenFile::getFree(pid,flags,nodeNo,devNo,node,file);
}

int VFS::stat(pid_t pid,const char *path,USER sFileInfo *info) {
	OpenFile *fsFile;
	const char *begin;
	int err = request(pid,path,VFS_READ,0,&begin,&fsFile);
	if(err < 0)
		return err;

	if(IS_NODE(fsFile)) {
		VFSNode *node = reinterpret_cast<VFSNode*>(fsFile);
		node->getInfo(pid,info);
		VFSNode::release(node);
	}
	else {
		err = VFSFS::stat(pid,fsFile,begin,info);
		info->device = fsFile->getNodeNo();
		MountSpace::release(fsFile);
	}
	return err;
}

int VFS::chmod(pid_t pid,const char *path,mode_t mode) {
	OpenFile *fsFile;
	const char *begin;
	int err = request(pid,path,VFS_WRITE,0,&begin,&fsFile);
	if(err < 0)
		return err;

	if(IS_NODE(fsFile)) {
		VFSNode *node = reinterpret_cast<VFSNode*>(fsFile);
		err = node->chmod(pid,mode);
		VFSNode::release(node);
	}
	else {
		err = VFSFS::chmod(pid,fsFile,begin,mode);
		MountSpace::release(fsFile);
	}
	return err;
}

int VFS::chown(pid_t pid,const char *path,uid_t uid,gid_t gid) {
	OpenFile *fsFile;
	const char *begin;
	int err = request(pid,path,VFS_WRITE,0,&begin,&fsFile);
	if(err < 0)
		return err;

	if(IS_NODE(fsFile)) {
		VFSNode *node = reinterpret_cast<VFSNode*>(fsFile);
		err = node->chown(pid,uid,gid);
		VFSNode::release(node);
	}
	else {
		err = VFSFS::chown(pid,fsFile,begin,uid,gid);
		MountSpace::release(fsFile);
	}
	return err;
}

int VFS::link(pid_t pid,const char *oldPath,const char *newPath) {
	char newPathCpy[MAX_PATH_LEN + 1];
	char *name,*namecpy,backup;
	size_t len;
	VFSNode *oldNode = NULL,*newNode = NULL;
	VFSNode *link;
	int res;

	OpenFile *oldFsFile = NULL,*newFsFile = NULL;
	const char *oldBegin,*newBegin;
	int oldRes = request(pid,oldPath,VFS_READ,0,&oldBegin,&oldFsFile);
	if(oldRes < 0)
		return oldRes;
	int newRes = request(pid,newPath,VFS_WRITE | VFS_NONODERES,0,&newBegin,&newFsFile);
	oldNode = reinterpret_cast<VFSNode*>(oldFsFile);
	newNode = reinterpret_cast<VFSNode*>(newFsFile);
	if(newRes < 0) {
		res = newRes;
		goto errorRelease;
	}

	if(!IS_NODE(oldFsFile)) {
		if(oldFsFile != newFsFile) {
			res = -EXDEV;
			goto errorRelease;
		}
		res = VFSFS::link(pid,oldFsFile,oldBegin,newBegin);
		MountSpace::release(oldFsFile);
		MountSpace::release(newFsFile);
		return res;
	}

	if(!IS_NODE(newFsFile)) {
		res = -EXDEV;
		goto errorRelease;
	}

	/* TODO prevent recursion? */

	/* copy path because we have to change it */
	len = strlen(newBegin);
	strcpy(newPathCpy,newBegin);
	/* check whether the directory exists */
	name = VFSNode::basename((char*)newPathCpy,&len);
	backup = *name;
	VFSNode::dirname((char*)newPathCpy,len);
	newRes = VFSNode::request(newPathCpy,&newNode,NULL,VFS_WRITE,0);
	if(newRes < 0) {
		res = -ENOENT;
		goto errorRelease;
	}

	/* links to directories not allowed */
	if(S_ISDIR(oldNode->getMode())) {
		res = -EISDIR;
		goto errorRelease;
	}

	/* make copy of name */
	*name = backup;
	len = strlen(name);
	namecpy = (char*)Cache::alloc(len + 1);
	if(namecpy == NULL) {
		res = -ENOMEM;
		goto errorRelease;
	}
	strcpy(namecpy,name);
	/* file exists? */
	if(newNode->findInDir(namecpy,len) != NULL) {
		res = -EEXIST;
		goto errorName;
	}
	/* check permissions */
	if((res = hasAccess(pid,newNode,VFS_WRITE)) < 0)
		goto errorName;
	/* now create link */
	if((link = createObj<VFSLink>(pid,newNode,namecpy,oldNode)) == NULL) {
		res = -ENOMEM;
		goto errorName;
	}
	VFSNode::release(link);
	VFSNode::release(newNode);
	VFSNode::release(oldNode);
	return 0;

errorName:
	Cache::free(namecpy);
errorRelease:
	if(IS_NODE(oldNode))
		VFSNode::release(oldNode);
	else
		MountSpace::release(oldFsFile);
	if(IS_NODE(newNode))
		VFSNode::release(newNode);
	else
		MountSpace::release(newFsFile);
	return res;
}

int VFS::unlink(pid_t pid,const char *path) {
	OpenFile *fsFile;
	const char *begin;
	int err = request(pid,path,VFS_WRITE | VFS_NOLINKRES,0,&begin,&fsFile);
	if(err < 0)
		return err;

	if(IS_NODE(fsFile)) {
		VFSNode *n = reinterpret_cast<VFSNode*>(fsFile);
		if(S_ISDIR(n->getMode())) {
			VFSNode::release(n);
			return -EISDIR;
		}

		/* check permissions */
		err = -EPERM;
		if(n->getOwner() == KERNEL_PID || (err = hasAccess(pid,n,VFS_WRITE)) < 0 ||
				IS_DEVICE(n->getMode())) {
			VFSNode::release(n);
			return err;
		}
		VFSNode::release(n);
		n->destroy();
	}
	else {
		err = VFSFS::unlink(pid,fsFile,begin);
		MountSpace::release(fsFile);
	}

	return err;
}

int VFS::mkdir(pid_t pid,const char *path) {
	char pathCpy[MAX_PATH_LEN + 1];
	char *name,*namecpy,backup;
	VFSNode *child;

	/* get the parent-directory */
	OpenFile *fsFile;
	const char *begin;
	int res = request(pid,path,VFS_WRITE | VFS_NONODERES,0,&begin,&fsFile);
	if(res < 0)
		return res;

	if(!IS_NODE(fsFile)) {
		res = VFSFS::mkdir(pid,fsFile,begin);
		MountSpace::release(fsFile);
		return res;
	}

	/* copy path because we'll change it */
	size_t len = strlen(begin);
	if(len >= MAX_PATH_LEN)
		return -ENAMETOOLONG;

	strcpy(pathCpy,begin);
	/* extract name and directory */
	name = VFSNode::basename(pathCpy,&len);
	backup = *name;
	VFSNode::dirname(pathCpy,len);

	/* get parent dir */
	VFSNode *node = reinterpret_cast<VFSNode*>(fsFile);
	res = VFSNode::request(pathCpy,&node,NULL,VFS_WRITE,0);
	if(res < 0)
		goto errorRel;

	/* alloc space for name and copy it over */
	*name = backup;
	len = strlen(name);
	namecpy = (char*)Cache::alloc(len + 1);
	if(namecpy == NULL) {
		res = -ENOMEM;
		goto errorRel;
	}
	strcpy(namecpy,name);

	/* does it exist? */
	if(node->findInDir(namecpy,len) != NULL) {
		res = -EEXIST;
		goto errorFree;
	}

	/* create dir */
	/* check permissions */
	if((res = hasAccess(pid,node,VFS_WRITE)) < 0)
		goto errorFree;
	child = createObj<VFSDir>(pid,node,namecpy,DIR_DEF_MODE);
	if(child == NULL) {
		res = -ENOMEM;
		goto errorFree;
	}

	VFSNode::release(child);
	VFSNode::release(node);
	return 0;

errorFree:
	Cache::free(namecpy);
errorRel:
	VFSNode::release(node);
	return res;
}

int VFS::rmdir(pid_t pid,const char *path) {
	OpenFile *fsFile;
	const char *begin;
	int res = request(pid,path,VFS_WRITE,0,&begin,&fsFile);
	if(res < 0)
		return res;

	if(!IS_NODE(fsFile)) {
		res = VFSFS::rmdir(pid,fsFile,begin);
		MountSpace::release(fsFile);
		return res;
	}

	/* check permissions */
	VFSNode *node = reinterpret_cast<VFSNode*>(fsFile);
	res = -EPERM;
	if(node->getOwner() == KERNEL_PID || (res = hasAccess(pid,node,VFS_WRITE)) < 0) {
		VFSNode::release(node);
		return res;
	}
	res = node->isEmptyDir();
	if(res < 0) {
		VFSNode::release(node);
		return res;
	}
	VFSNode::release(node);
	node->destroy();
	return 0;
}

int VFS::createdev(pid_t pid,char *path,mode_t mode,uint type,uint ops,OpenFile **file) {
	VFSNode *dir = NULL,*srv;

	/* get name */
	size_t len = strlen(path);
	char *name = VFSNode::basename(path,&len);
	name = strdup(name);
	if(!name)
		return -ENOMEM;

	/* check whether the directory exists */
	VFSNode::dirname(path,len);
	int err = VFSNode::request(path,&dir,NULL,VFS_READ,0);
	if(err < 0) {
		Cache::free(name);
		return err;
	}

	/* ensure its a directory */
	if(!S_ISDIR(dir->getMode()))
		goto errorDir;

	/* check whether the device does already exist */
	if(dir->findInDir(name,strlen(name)) != NULL) {
		err = -EEXIST;
		goto errorDir;
	}

	/* create node */
	srv = createObj<VFSDevice>(pid,dir,name,mode,type,ops);
	if(!srv) {
		err = -ENOMEM;
		goto errorDir;
	}
	err = openFile(pid,VFS_DEVICE,srv,srv->getNo(),VFS_DEV_NO,file);
	if(err < 0)
		goto errDevice;
	VFSNode::release(srv);
	VFSNode::release(dir);
	return err;

errDevice:
	VFSNode::release(srv);
	VFSNode::release(srv);
errorDir:
	VFSNode::release(dir);
	/* the release has already free'd the name */
	return err;
}

inode_t VFS::createProcess(pid_t pid) {
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

	/* create mountspace-info-node */
	nn = createObj<VFSInfo::MountSpaceFile>(KERNEL_PID,dir);
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
	/* remove from /system/processes */
	const Proc *p = Proc::getByPid(pid);
	VFSNode *node = VFSNode::get(p->getThreadsDir());
	node->getParent()->destroy();
}

inode_t VFS::createThread(tid_t tid) {
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
	VFSNode *n = VFSNode::get(t->getThreadDir());
	n->destroy();
}

void VFS::printMsgs(OStream &os) {
	bool isValid;
	const VFSNode *drv = devNode->openDir(true,&isValid);
	if(isValid) {
		os.writef("Messages:\n");
		while(drv != NULL) {
			if(IS_DEVICE(drv->getMode())) {
				os.pushIndent();
				drv->print(os);
				os.popIndent();
			}
			drv = drv->next;
		}
	}
	devNode->closeDir(true);
}
