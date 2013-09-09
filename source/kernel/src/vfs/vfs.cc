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
#include <sys/task/event.h>
#include <sys/task/lock.h>
#include <sys/task/timer.h>
#include <sys/mem/paging.h>
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
klock_t waitLock;

void VFS::init() {
	VFSNode *root,*sys;

	/*
	 *  /
	 *   |- system
	 *   |   |- pipe
	 *   |   |- mbmods
	 *   |   |- shm
	 *   |   |- devices
	 *   |   \- processes
	 *   |       \- self
	 *   \- dev
	 */
	root = CREATE(VFSDir,KERNEL_PID,nullptr,(char*)"");
	sys = CREATE(VFSDir,KERNEL_PID,root,(char*)"system");
	VFSNode::release(CREATE(VFSDir,KERNEL_PID,sys,(char*)"pipe"));
	VFSNode::release(CREATE(VFSDir,KERNEL_PID,sys,(char*)"mbmods"));
	VFSNode::release(CREATE(VFSDir,KERNEL_PID,sys,(char*)"shm"));
	procsNode = CREATE(VFSDir,KERNEL_PID,sys,(char*)"processes");
	VFSNode::release(CREATE(VFSSelfLink,KERNEL_PID,procsNode,(char*)"self"));
	VFSNode::release(CREATE(VFSDir,KERNEL_PID,sys,(char*)"devices"));
	devNode = CREATE(VFSDir,KERNEL_PID,root,(char*)"dev");
	VFSNode::release(devNode);
	VFSNode::release(procsNode);
	VFSNode::release(sys);
	VFSNode::release(root);

	VFSInfo::init();
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

int VFS::openPath(pid_t pid,ushort flags,const char *path,OpenFile **file) {
	/* resolve path */
	bool created;
	VFSNode *node;
	int err = VFSNode::request(path,&node,&created,flags);
	if(err == -EREALPATH) {
		const Proc *p = Proc::getByPid(pid);
		/* unfortunatly we have to check for fs here. because e.g. if the user tries to mount the
		 * device "/realfile" the userspace has no opportunity to distinguish between virtual
		 * and real files. therefore fs will try to open this path and shoot itself in the foot... */
		/* TODO there has to be a better solution */
		if(p->getFlags() & P_FS)
			return -ENOENT;

		/* send msg to fs and wait for reply */
		err = VFSFS::openPath(pid,flags,path,file);
		if(err < 0)
			return err;

		/* store the path for debugging purposes */
		(*file)->setPath(strdup(path));
	}
	else {
		/* handle virtual files */
		if(err < 0)
			return err;

		/* if its a device, create the channel-node */
		inode_t nodeNo = node->getNo();
		if(IS_DEVICE(node->getMode())) {
			VFSNode *child;
			/* check if we can access the device */
			if((err = hasAccess(pid,node,flags)) < 0) {
				VFSNode::release(node);
				return err;
			}
			child = CREATE(VFSChannel,pid,node);
			if(child == NULL) {
				VFSNode::release(node);
				return -ENOMEM;
			}
			VFSNode::release(node);
			node = child;
		}

		/* open file */
		err = openFile(pid,flags,node,nodeNo,VFS_DEV_NO,file);
		if(err < 0) {
			VFSNode::release(node);
			return err;
		}

		/* give the node a chance to react on it */
		err = node->open(pid,*file,flags);
		if(err < 0) {
			/* close removes the channel device-node, if it is one */
			(*file)->close(pid);
			VFSNode::release(node);
			return err;
		}
		VFSNode::release(node);
	}

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
	VFSNode *node;
	int err = VFSNode::request("/system/pipe",&node,NULL,VFS_READ);
	if(err < 0)
		return err;

	/* create pipe */
	VFSNode *pipeNode = CREATE(VFSPipe,pid,node);
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

	if(node && (err = hasAccess(pid,node,flags)) < 0)
		return err;

	/* determine free file */
	return OpenFile::getFree(pid,flags,nodeNo,devNo,node,file);
}

int VFS::stat(pid_t pid,const char *path,USER sFileInfo *info) {
	VFSNode *node;
	int res = VFSNode::request(path,&node,NULL,VFS_READ);
	if(res == -EREALPATH)
		res = VFSFS::stat(pid,path,info);
	else if(res == 0) {
		node->getInfo(pid,info);
		VFSNode::release(node);
	}
	return res;
}

int VFS::chmod(pid_t pid,const char *path,mode_t mode) {
	VFSNode *node;
	int res = VFSNode::request(path,&node,NULL,VFS_READ);
	if(res == -EREALPATH)
		res = VFSFS::chmod(pid,path,mode);
	else if(res == 0) {
		res = node->chmod(pid,mode);
		VFSNode::release(node);
	}
	return res;
}

int VFS::chown(pid_t pid,const char *path,uid_t uid,gid_t gid) {
	VFSNode *node;
	int res = VFSNode::request(path,&node,NULL,VFS_READ);
	if(res == -EREALPATH)
		res = VFSFS::chown(pid,path,uid,gid);
	else if(res == 0) {
		res = node->chown(pid,uid,gid);
		VFSNode::release(node);
	}
	return res;
}

bool VFS::hasMsg(VFSNode *node) {
	return IS_CHANNEL(node->getMode()) && static_cast<VFSChannel*>(node)->hasReply();
}

bool VFS::hasData(VFSNode *node) {
	return IS_DEVICE(node->getParent()->getMode()) &&
			static_cast<VFSDevice*>(node->getParent())->isReadable();
}

bool VFS::hasWork(VFSNode *node) {
	return IS_DEVICE(node->getMode()) && static_cast<VFSDevice*>(node)->hasWork();
}

int VFS::waitFor(Event::WaitObject *objects,size_t objCount,time_t maxWaitTime,bool block,
		pid_t pid,ulong ident) {
	Thread *t = Thread::getRunning();
	bool isFirstWait = true;
	int res;

	/* transform the files into vfs-nodes */
	for(size_t i = 0; i < objCount; i++) {
		if(objects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) {
			OpenFile *file = (OpenFile*)objects[i].object;
			if(file->getDev() != VFS_DEV_NO)
				return -EPERM;
			objects[i].object = (evobj_t)file->getNode();
		}
	}

	while(true) {
		/* we have to lock this region to ensure that if we've found out that we can sleep, no one
		 * sends us an event before we've finished the Event::waitObjects(). otherwise, it would be
		 * possible that we never wake up again, because we have missed the event and get no other
		 * one. */
		SpinLock::acquire(&waitLock);
		/* check whether we can wait */
		for(size_t i = 0; i < objCount; i++) {
			if(objects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) {
				VFSNode *n = (VFSNode*)objects[i].object;
				if(!n->isAlive()) {
					SpinLock::release(&waitLock);
					res = -EDESTROYED;
					goto error;
				}
				if((objects[i].events & EV_CLIENT) && hasWork(n))
					goto noWait;
				else if((objects[i].events & EV_RECEIVED_MSG) && hasMsg(n))
					goto noWait;
				else if((objects[i].events & EV_DATA_READABLE) && hasData(n))
					goto noWait;
			}
		}

		if(!block) {
			SpinLock::release(&waitLock);
			return -EWOULDBLOCK;
		}

		/* wait */
		if(!Event::waitObjects(t,objects,objCount)) {
			SpinLock::release(&waitLock);
			res = -ENOMEM;
			goto error;
		}
		if(pid != KERNEL_PID)
			Lock::release(pid,ident);
		if(isFirstWait && maxWaitTime != 0)
			Timer::sleepFor(t->getTid(),maxWaitTime,true);
		SpinLock::release(&waitLock);

		Thread::switchAway();
		if(Signals::hasSignalFor(t->getTid())) {
			res = -EINTR;
			goto error;
		}
		/* if we're waiting for other events, too, we have to wake up */
		for(size_t i = 0; i < objCount; i++) {
			if((objects[i].events & ~(EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)))
				goto done;
		}
		isFirstWait = false;
	}

noWait:
	if(pid != KERNEL_PID)
		Lock::release(pid,ident);
	SpinLock::release(&waitLock);
done:
	res = 0;
error:
	if(maxWaitTime != 0)
		Timer::removeThread(t->getTid());
	return res;
}

int VFS::mount(pid_t pid,const char *device,const char *path,uint type) {
	VFSNode *node;
	int err = VFSNode::request(path,&node,NULL,VFS_READ);
	if(err != -EREALPATH) {
		if(err == 0)
			VFSNode::release(node);
		return -EPERM;
	}
	return VFSFS::mount(pid,device,path,type);
}

int VFS::unmount(pid_t pid,const char *path) {
	VFSNode *node;
	int err = VFSNode::request(path,&node,NULL,VFS_READ);
	if(err != -EREALPATH) {
		if(err == 0)
			VFSNode::release(node);
		return -EPERM;
	}
	return VFSFS::unmount(pid,path);
}

int VFS::sync(pid_t pid) {
	return VFSFS::sync(pid);
}

int VFS::link(pid_t pid,const char *oldPath,const char *newPath) {
	char newPathCpy[MAX_PATH_LEN + 1];
	char *name,*namecpy,backup;
	size_t len;
	VFSNode *oldNode,*newNode,*link;
	int res;

	/* first check whether it is a realpath */
	int oldRes = VFSNode::request(oldPath,&oldNode,NULL,VFS_READ);
	int newRes = VFSNode::request(newPath,&newNode,NULL,VFS_WRITE);
	if(oldRes == -EREALPATH) {
		if(newRes != -EREALPATH) {
			res = -EXDEV;
			goto errorRelease;
		}
		return VFSFS::link(pid,oldPath,newPath);
	}
	if(oldRes < 0) {
		res = oldRes;
		goto errorRelease;
	}
	if(newRes >= 0) {
		res = -EEXIST;
		goto errorRelease;
	}

	/* TODO prevent recursion? */

	/* copy path because we have to change it */
	len = strlen(newPath);
	strcpy(newPathCpy,newPath);
	/* check whether the directory exists */
	name = VFSNode::basename((char*)newPathCpy,&len);
	backup = *name;
	VFSNode::dirname((char*)newPathCpy,len);
	newRes = VFSNode::request(newPathCpy,&newNode,NULL,VFS_WRITE);
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
	if((link = CREATE(VFSLink,pid,newNode,namecpy,oldNode)) == NULL) {
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
	VFSNode::release(oldNode);
	VFSNode::release(newNode);
	return res;
}

int VFS::unlink(pid_t pid,const char *path) {
	VFSNode *n;
	int res = VFSNode::request(path,&n,NULL,VFS_WRITE | VFS_NOLINKRES);
	if(res == -EREALPATH)
		return VFSFS::unlink(pid,path);
	if(res < 0)
		return -ENOENT;

	if(S_ISDIR(n->getMode())) {
		VFSNode::release(n);
		return -EISDIR;
	}

	/* check permissions */
	res = -EPERM;
	if(n->getOwner() == KERNEL_PID || (res = hasAccess(pid,n,VFS_WRITE)) < 0 ||
			IS_DEVICE(n->getMode())) {
		VFSNode::release(n);
		return res;
	}
	VFSNode::release(n);
	n->destroy();
	return 0;
}

int VFS::mkdir(pid_t pid,const char *path) {
	char pathCpy[MAX_PATH_LEN + 1];
	size_t len = strlen(path);
	VFSNode *node,*child;

	/* copy path because we'll change it */
	if(len >= MAX_PATH_LEN)
		return -ENAMETOOLONG;
	strcpy(pathCpy,path);

	/* extract name and directory */
	char *name = VFSNode::basename(pathCpy,&len);
	char backup = *name;
	VFSNode::dirname(pathCpy,len);

	/* get the parent-directory */
	int res = VFSNode::request(pathCpy,&node,NULL,VFS_WRITE);
	/* special-case: directories in / should be created in the real fs! */
	if(res == -EREALPATH || (res >= 0 && strcmp(pathCpy,"/") == 0)) {
		VFSNode::release(node);
		/* let fs handle the request */
		return VFSFS::mkdir(pid,path);
	}
	if(res < 0)
		return res;

	/* alloc space for name and copy it over */
	*name = backup;
	len = strlen(name);
	char *namecpy = (char*)Cache::alloc(len + 1);
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
	child = CREATE(VFSDir,pid,node,namecpy);
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
	VFSNode *node;
	int res = VFSNode::request(path,&node,NULL,VFS_WRITE);
	if(res == -EREALPATH)
		return VFSFS::rmdir(pid,path);
	if(res < 0)
		return -ENOENT;

	/* check permissions */
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

int VFS::createdev(pid_t pid,char *path,uint type,uint ops,OpenFile **file) {
	VFSNode *dir,*srv;

	/* get name */
	size_t len = strlen(path);
	char *name = VFSNode::basename(path,&len);
	name = strdup(name);
	if(!name)
		return -ENOMEM;

	/* check whether the directory exists */
	VFSNode::dirname(path,len);
	int err = VFSNode::request(path,&dir,NULL,VFS_READ);
	/* this includes -EREALPATH since devices have to be created in the VFS */
	if(err < 0)
		goto errorName;

	/* ensure its a directory */
	if(!S_ISDIR(dir->getMode()))
		goto errorDir;

	/* check whether the device does already exist */
	if(dir->findInDir(name,strlen(name)) != NULL) {
		err = -EEXIST;
		goto errorDir;
	}

	/* create node */
	srv = CREATE(VFSDevice,pid,dir,name,type,ops);
	if(!srv) {
		err = -ENOMEM;
		goto errorDir;
	}
	err = openFile(pid,VFS_MSGS | VFS_DEVICE,srv,srv->getNo(),VFS_DEV_NO,file);
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
errorName:
	Cache::free(name);
	return err;
}

inode_t VFS::createProcess(pid_t pid) {
	VFSNode *proc = procsNode,*dir,*nn;
	const VFSNode *n;
	int res = -ENOMEM;

	/* build name */
	char *name = (char*)Cache::alloc(12);
	if(name == NULL)
		return -ENOMEM;

	itoa(name,12,pid);

	/* TODO remove that */
	/* go to last entry */
	bool isValid;
	n = proc->openDir(true,&isValid);
	if(!isValid) {
		proc->closeDir(true);
		res = -EDESTROYED;
		goto errorName;
	}
	while(n != NULL) {
		/* entry already existing? */
		if(strcmp(n->getName(),name) == 0) {
			proc->closeDir(true);
			res = -EEXIST;
			goto errorName;
		}
		n = n->next;
	}
	proc->closeDir(true);

	/* create dir */
	dir = CREATE(VFSDir,KERNEL_PID,proc,name);
	if(dir == NULL)
		goto errorName;

	/* create process-info-node */
	nn = CREATE(VFSInfo::ProcFile,KERNEL_PID,dir);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create virt-mem-info-node */
	nn = CREATE(VFSInfo::VirtMemFile,KERNEL_PID,dir);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create regions-info-node */
	nn = CREATE(VFSInfo::RegionsFile,KERNEL_PID,dir);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create maps-info-node */
	nn = CREATE(VFSInfo::MapFile,KERNEL_PID,dir);
	if(nn == NULL)
		goto errorDir;
	VFSNode::release(nn);

	/* create threads-dir */
	nn = CREATE(VFSDir,KERNEL_PID,dir,(char*)"threads");
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
	VFSFS::removeProc(pid);
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
	dir = CREATE(VFSDir,KERNEL_PID,n,name);
	if(dir == NULL) {
		Cache::free(name);
		goto errorDir;
	}

	/* create info-node */
	n = CREATE(VFSInfo::ThreadFile,KERNEL_PID,dir);
	if(n == NULL)
		goto errorInfo;
	VFSNode::release(n);

	/* create trace-node */
	n = CREATE(VFSInfo::TraceFile,KERNEL_PID,dir);
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
	VFSNode *n= VFSNode::get(t->getThreadDir());
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
