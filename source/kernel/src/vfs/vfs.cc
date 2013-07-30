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
#include <sys/vfs/fsmsgs.h>
#include <sys/vfs/info.h>
#include <sys/vfs/devmsgs.h>
#include <sys/vfs/file.h>
#include <sys/vfs/dir.h>
#include <sys/vfs/link.h>
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
#include <esc/messages.h>
#include <esc/sllist.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

static sVFSNode *procsNode;
static sVFSNode *devNode;
klock_t waitLock;

void vfs_init(void) {
	sVFSNode *root,*sys;

	/*
	 *  /
	 *   |- system
	 *   |   |-pipe
	 *   |   |-mbmods
	 *   |   |-shm
	 *   |   |-processes
	 *   |   \-devices
	 *   \- dev
	 */
	root = vfs_dir_create(KERNEL_PID,NULL,(char*)"");
	sys = vfs_dir_create(KERNEL_PID,root,(char*)"system");
	vfs_dir_create(KERNEL_PID,sys,(char*)"pipe");
	vfs_dir_create(KERNEL_PID,sys,(char*)"mbmods");
	vfs_dir_create(KERNEL_PID,sys,(char*)"shm");
	procsNode = vfs_dir_create(KERNEL_PID,sys,(char*)"processes");
	vfs_dir_create(KERNEL_PID,sys,(char*)"devices");
	devNode = vfs_dir_create(KERNEL_PID,root,(char*)"dev");

	vfs_info_init();
}

int vfs_hasAccess(pid_t pid,sVFSNode *n,ushort flags) {
	const Proc *p;
	uint mode;
	if(n->name == NULL)
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
			return (n->mode & MODE_EXEC) ? 0 : -EACCES;
		return 0;
	}

	/* determine mask */
	if(p->getEUid() == n->uid)
		mode = n->mode & S_IRWXU;
	else if(p->getEGid() == n->gid || Groups::contains(p->getPid(),n->gid))
		mode = n->mode & S_IRWXG;
	else
		mode = n->mode & S_IRWXO;

	/* check access */
	if((flags & VFS_READ) && !(mode & MODE_READ))
		return -EACCES;
	if((flags & VFS_WRITE) && !(mode & MODE_WRITE))
		return -EACCES;
	if((flags & VFS_EXEC) && !(mode & MODE_EXEC))
		return -EACCES;
	return 0;
}

int vfs_openPath(pid_t pid,ushort flags,const char *path,OpenFile **file) {
	inode_t nodeNo;
	bool created;
	int err;

	/* resolve path */
	err = vfs_node_resolvePath(path,&nodeNo,&created,flags);
	if(err == -EREALPATH) {
		const Proc *p = Proc::getByPid(pid);
		/* unfortunatly we have to check for fs here. because e.g. if the user tries to mount the
		 * device "/realfile" the userspace has no opportunity to distinguish between virtual
		 * and real files. therefore fs will try to open this path and shoot itself in the foot... */
		/* TODO there has to be a better solution */
		if(p->getFlags() & P_FS)
			return -ENOENT;

		/* send msg to fs and wait for reply */
		err = vfs_fsmsgs_openPath(pid,flags,path,file);
		if(err < 0)
			return err;

		/* store the path for debugging purposes */
		(*file)->setPath(strdup(path));
	}
	else {
		sVFSNode *node;
		/* handle virtual files */
		if(err < 0)
			return err;

		node = vfs_node_request(nodeNo);
		if(!node)
			return -ENOENT;

		/* if its a device, create the channel-node */
		if(IS_DEVICE(node->mode)) {
			sVFSNode *child;
			/* check if we can access the device */
			if((err = vfs_hasAccess(pid,node,flags)) < 0) {
				vfs_node_release(node);
				return err;
			}
			child = vfs_chan_create(pid,node);
			if(child == NULL) {
				vfs_node_release(node);
				return -ENOMEM;
			}
			nodeNo = vfs_node_getNo(child);
		}
		vfs_node_release(node);

		/* open file */
		err = vfs_openFile(pid,flags,nodeNo,VFS_DEV_NO,file);
		if(err < 0)
			return err;

		/* if it is a device, call the device open-command; no node-request here because we have
		 * a file for it */
		node = vfs_node_get(nodeNo);
		if(IS_CHANNEL(node->mode)) {
			err = vfs_devmsgs_open(pid,*file,node,flags);
			if(err < 0) {
				/* close removes the channeldevice-node, if it is one */
				(*file)->closeFile(pid);
				return err;
			}
		}
	}

	/* append? */
	if(flags & VFS_APPEND) {
		err = (*file)->seek(pid,0,SEEK_END);
		if(err < 0) {
			(*file)->closeFile(pid);
			return err;
		}
	}
	return 0;
}

int vfs_openPipe(pid_t pid,OpenFile **readFile,OpenFile **writeFile) {
	sVFSNode *node,*pipeNode;
	inode_t nodeNo,pipeNodeNo;
	int err;

	/* resolve pipe-path */
	err = vfs_node_resolvePath("/system/pipe",&nodeNo,NULL,VFS_READ);
	if(err < 0)
		return err;

	/* create pipe */
	node = vfs_node_request(nodeNo);
	pipeNode = vfs_pipe_create(pid,node);
	vfs_node_release(node);
	if(pipeNode == NULL)
		return -ENOMEM;

	pipeNodeNo = vfs_node_getNo(pipeNode);
	/* open file for reading */
	err = vfs_openFile(pid,VFS_READ,pipeNodeNo,VFS_DEV_NO,readFile);
	if(err < 0) {
		vfs_node_destroy(pipeNode);
		return err;
	}

	/* open file for writing */
	err = vfs_openFile(pid,VFS_WRITE,pipeNodeNo,VFS_DEV_NO,writeFile);
	if(err < 0) {
		/* closeFile removes the pipenode, too */
		(*readFile)->closeFile(pid);
		return err;
	}
	return 0;
}

int vfs_openFile(pid_t pid,ushort flags,inode_t nodeNo,dev_t devNo,OpenFile **file) {
	sVFSNode *n = NULL;
	int err;

	/* cleanup flags */
	flags &= VFS_READ | VFS_WRITE | VFS_MSGS | VFS_NOBLOCK | VFS_DEVICE | VFS_EXCLUSIVE;

	if(devNo == VFS_DEV_NO) {
		n = vfs_node_request(nodeNo);
		if(!n)
			return -ENOENT;
		if((err = vfs_hasAccess(pid,n,flags)) < 0) {
			vfs_node_release(n);
			return err;
		}
	}

	/* determine free file */
	err = OpenFile::getFree(pid,flags,nodeNo,devNo,n,file);
	if(devNo == VFS_DEV_NO)
		vfs_node_release(n);
	return err;
}

int vfs_stat(pid_t pid,const char *path,USER sFileInfo *info) {
	inode_t nodeNo;
	int res = vfs_node_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res == -EREALPATH)
		res = vfs_fsmsgs_stat(pid,path,info);
	else if(res == 0)
		res = vfs_node_getInfo(pid,nodeNo,info);
	return res;
}

int vfs_chmod(pid_t pid,const char *path,mode_t mode) {
	inode_t nodeNo;
	int res = vfs_node_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res == -EREALPATH)
		res = vfs_fsmsgs_chmod(pid,path,mode);
	else if(res == 0)
		res = vfs_node_chmod(pid,nodeNo,mode);
	return res;
}

int vfs_chown(pid_t pid,const char *path,uid_t uid,gid_t gid) {
	inode_t nodeNo;
	int res = vfs_node_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res == -EREALPATH)
		res = vfs_fsmsgs_chown(pid,path,uid,gid);
	else if(res == 0)
		res = vfs_node_chown(pid,nodeNo,uid,gid);
	return res;
}

static bool vfs_hasMsg(sVFSNode *node) {
	return IS_CHANNEL(node->mode) && vfs_chan_hasReply(node);
}

static bool vfs_hasData(sVFSNode *node) {
	return IS_DEVICE(node->parent->mode) && vfs_device_isReadable(node->parent);
}

static bool vfs_hasWork(sVFSNode *node) {
	return IS_DEVICE(node->mode) && vfs_device_hasWork(node);
}

int vfs_waitFor(Event::WaitObject *objects,size_t objCount,time_t maxWaitTime,bool block,
		pid_t pid,ulong ident) {
	Thread *t = Thread::getRunning();
	size_t i;
	bool isFirstWait = true;
	int res;

	/* transform the files into vfs-nodes */
	for(i = 0; i < objCount; i++) {
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
		for(i = 0; i < objCount; i++) {
			if(objects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) {
				sVFSNode *n = (sVFSNode*)objects[i].object;
				if(n->name == NULL) {
					SpinLock::release(&waitLock);
					res = -EDESTROYED;
					goto error;
				}
				if((objects[i].events & EV_CLIENT) && vfs_hasWork(n))
					goto noWait;
				else if((objects[i].events & EV_RECEIVED_MSG) && vfs_hasMsg(n))
					goto noWait;
				else if((objects[i].events & EV_DATA_READABLE) && vfs_hasData(n))
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
		for(i = 0; i < objCount; i++) {
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

int vfs_mount(pid_t pid,const char *device,const char *path,uint type) {
	inode_t ino;
	if(vfs_node_resolvePath(path,&ino,NULL,VFS_READ) != -EREALPATH)
		return -EPERM;
	return vfs_fsmsgs_mount(pid,device,path,type);
}

int vfs_unmount(pid_t pid,const char *path) {
	inode_t ino;
	if(vfs_node_resolvePath(path,&ino,NULL,VFS_READ) != -EREALPATH)
		return -EPERM;
	return vfs_fsmsgs_unmount(pid,path);
}

int vfs_sync(pid_t pid) {
	return vfs_fsmsgs_sync(pid);
}

int vfs_link(pid_t pid,const char *oldPath,const char *newPath) {
	char newPathCpy[MAX_PATH_LEN + 1];
	char *name,*namecpy,backup;
	size_t len;
	inode_t oldIno,newIno;
	sVFSNode *dir,*target;
	int res,oldRes,newRes;
	/* first check whether it is a realpath */
	oldRes = vfs_node_resolvePath(oldPath,&oldIno,NULL,VFS_READ);
	newRes = vfs_node_resolvePath(newPath,&newIno,NULL,VFS_WRITE);
	if(oldRes == -EREALPATH) {
		if(newRes != -EREALPATH)
			return -EXDEV;
		return vfs_fsmsgs_link(pid,oldPath,newPath);
	}
	if(oldRes < 0)
		return oldRes;
	if(newRes >= 0)
		return -EEXIST;

	/* TODO prevent recursion? */

	/* copy path because we have to change it */
	len = strlen(newPath);
	strcpy(newPathCpy,newPath);
	/* check whether the directory exists */
	name = vfs_node_basename((char*)newPathCpy,&len);
	backup = *name;
	vfs_node_dirname((char*)newPathCpy,len);
	newRes = vfs_node_resolvePath(newPathCpy,&newIno,NULL,VFS_WRITE);
	if(newRes < 0)
		return -ENOENT;

	/* links to directories not allowed */
	target = vfs_node_request(oldIno);
	if(!target)
		return -ENOENT;
	if(S_ISDIR(target->mode)) {
		res = -EISDIR;
		goto errorTarget;
	}

	/* make copy of name */
	*name = backup;
	len = strlen(name);
	namecpy = (char*)Cache::alloc(len + 1);
	if(namecpy == NULL) {
		res = -ENOMEM;
		goto errorTarget;
	}
	strcpy(namecpy,name);
	/* file exists? */
	if(vfs_node_findInDirOf(newIno,namecpy,len) != NULL) {
		res = -EEXIST;
		goto errorName;
	}
	/* now create link */
	dir = vfs_node_request(newIno);
	if(!dir) {
		res = -ENOENT;
		goto errorName;
	}
	/* check permissions */
	if((res = vfs_hasAccess(pid,dir,VFS_WRITE)) < 0)
		goto errorDir;
	if(vfs_link_create(pid,dir,namecpy,target) == NULL) {
		res = -ENOMEM;
		goto errorDir;
	}
	vfs_node_release(dir);
	return 0;

errorDir:
	vfs_node_release(dir);
errorName:
	Cache::free(namecpy);
errorTarget:
	vfs_node_release(target);
	return res;
}

int vfs_unlink(pid_t pid,const char *path) {
	int res;
	inode_t ino;
	sVFSNode *n;
	res = vfs_node_resolvePath(path,&ino,NULL,VFS_WRITE | VFS_NOLINKRES);
	if(res == -EREALPATH)
		return vfs_fsmsgs_unlink(pid,path);
	if(res < 0)
		return -ENOENT;

	n = vfs_node_request(ino);
	if(!n)
		return -ENOENT;
	/* check permissions */
	res = -EPERM;
	if(n->owner == KERNEL_PID || (res = vfs_hasAccess(pid,n,VFS_WRITE)) < 0) {
		vfs_node_release(n);
		return res;
	}
	vfs_node_release(n);
	vfs_node_destroyNow(n);
	return 0;
}

int vfs_mkdir(pid_t pid,const char *path) {
	char pathCpy[MAX_PATH_LEN + 1];
	char *name,*namecpy;
	char backup;
	int res;
	size_t len = strlen(path);
	inode_t inodeNo;
	sVFSNode *node,*child;

	/* copy path because we'll change it */
	if(len >= MAX_PATH_LEN)
		return -ENAMETOOLONG;
	strcpy(pathCpy,path);

	/* extract name and directory */
	name = vfs_node_basename(pathCpy,&len);
	backup = *name;
	vfs_node_dirname(pathCpy,len);

	/* get the parent-directory */
	res = vfs_node_resolvePath(pathCpy,&inodeNo,NULL,VFS_WRITE);
	/* special-case: directories in / should be created in the real fs! */
	if(res == -EREALPATH || (res >= 0 && strcmp(pathCpy,"/") == 0)) {
		/* let fs handle the request */
		return vfs_fsmsgs_mkdir(pid,path);
	}
	if(res < 0)
		return res;

	/* alloc space for name and copy it over */
	*name = backup;
	len = strlen(name);
	namecpy = (char*)Cache::alloc(len + 1);
	if(namecpy == NULL)
		return -ENOMEM;
	strcpy(namecpy,name);
	/* does it exist? */
	if(vfs_node_findInDirOf(inodeNo,namecpy,len) != NULL) {
		res = -EEXIST;
		goto error;
	}
	/* create dir */
	node = vfs_node_request(inodeNo);
	if(!node) {
		res = -ENOENT;
		goto error;
	}
	/* check permissions */
	if((res = vfs_hasAccess(pid,node,VFS_WRITE)) < 0)
		goto errorRel;
	child = vfs_dir_create(pid,node,namecpy);
	if(child == NULL) {
		res = -ENOMEM;
		goto errorRel;
	}
	vfs_node_release(node);
	return 0;

errorRel:
	vfs_node_release(node);
error:
	Cache::free(namecpy);
	return res;
}

int vfs_rmdir(pid_t pid,const char *path) {
	int res;
	sVFSNode *node;
	inode_t inodeNo;
	res = vfs_node_resolvePath(path,&inodeNo,NULL,VFS_WRITE);
	if(res == -EREALPATH)
		return vfs_fsmsgs_rmdir(pid,path);
	if(res < 0)
		return -ENOENT;

	node = vfs_node_request(inodeNo);
	if(!node)
		return -ENOENT;
	/* check permissions */
	res = -EPERM;
	if(node->owner == KERNEL_PID || (res = vfs_hasAccess(pid,node,VFS_WRITE)) < 0) {
		vfs_node_release(node);
		return res;
	}
	res = vfs_node_isEmptyDir(node);
	if(res < 0) {
		vfs_node_release(node);
		return res;
	}
	vfs_node_release(node);
	vfs_node_destroyNow(node);
	return 0;
}

int vfs_createdev(pid_t pid,char *path,uint type,uint ops,OpenFile **file) {
	sVFSNode *dir,*srv;
	size_t len;
	char *name;
	inode_t nodeNo;
	int err;

	/* get name */
	len = strlen(path);
	name = vfs_node_basename(path,&len);
	name = strdup(name);
	if(!name)
		return -ENOMEM;

	/* check whether the directory exists */
	vfs_node_dirname(path,len);
	err = vfs_node_resolvePath(path,&nodeNo,NULL,VFS_READ);
	/* this includes -EREALPATH since devices have to be created in the VFS */
	if(err < 0)
		goto errorName;

	/* ensure its a directory */
	dir = vfs_node_request(nodeNo);
	if(!dir)
		goto errorName;
	if(!S_ISDIR(dir->mode))
		goto errorDir;

	/* check whether the device does already exist */
	if(vfs_node_findInDir(dir,name,strlen(name)) != NULL) {
		err = -EEXIST;
		goto errorDir;
	}

	/* create node */
	srv = vfs_device_create(pid,dir,name,type,ops);
	if(!srv) {
		err = -ENOMEM;
		goto errorDir;
	}
	err = vfs_openFile(pid,VFS_MSGS | VFS_DEVICE,vfs_node_getNo(srv),VFS_DEV_NO,file);
	if(err < 0)
		goto errDevice;
	vfs_node_release(dir);
	return err;

errDevice:
	vfs_node_destroy(srv);
errorDir:
	vfs_node_release(dir);
errorName:
	Cache::free(name);
	return err;
}

inode_t vfs_createProcess(pid_t pid) {
	char *name;
	sVFSNode *proc = procsNode;
	sVFSNode *n;
	sVFSNode *dir,*tdir;
	bool isValid;
	int res = -ENOMEM;

	/* build name */
	name = (char*)Cache::alloc(12);
	if(name == NULL)
		return -ENOMEM;

	itoa(name,12,pid);

	/* go to last entry */
	n = vfs_node_openDir(proc,true,&isValid);
	if(!isValid) {
		vfs_node_closeDir(proc,true);
		res = -EDESTROYED;
		goto errorName;
	}
	while(n != NULL) {
		/* entry already existing? */
		if(strcmp(n->name,name) == 0) {
			vfs_node_closeDir(proc,true);
			res = -EEXIST;
			goto errorName;
		}
		n = n->next;
	}
	vfs_node_closeDir(proc,true);

	/* create dir */
	dir = vfs_dir_create(KERNEL_PID,proc,name);
	if(dir == NULL)
		goto errorName;

	dir = vfs_node_request(vfs_node_getNo(dir));
	if(!dir)
		goto errorName;
	/* create process-info-node */
	n = vfs_file_create(KERNEL_PID,dir,(char*)"info",vfs_info_procReadHandler,NULL);
	if(n == NULL)
		goto errorDir;

	/* create virt-mem-info-node */
	n = vfs_file_create(KERNEL_PID,dir,(char*)"virtmem",vfs_info_virtMemReadHandler,NULL);
	if(n == NULL)
		goto errorDir;

	/* create regions-info-node */
	n = vfs_file_create(KERNEL_PID,dir,(char*)"regions",vfs_info_regionsReadHandler,NULL);
	if(n == NULL)
		goto errorDir;

	/* create maps-info-node */
	n = vfs_file_create(KERNEL_PID,dir,(char*)"maps",vfs_info_mapsReadHandler,NULL);
	if(n == NULL)
		goto errorDir;

	/* create threads-dir */
	tdir = vfs_dir_create(KERNEL_PID,dir,(char*)"threads");
	if(tdir == NULL)
		goto errorDir;

	vfs_node_release(dir);
	return vfs_node_getNo(tdir);

errorDir:
	vfs_node_release(dir);
	vfs_node_destroy(dir);
errorName:
	Cache::free(name);
	return res;
}

void vfs_removeProcess(pid_t pid) {
	/* remove from /system/processes */
	const Proc *p = Proc::getByPid(pid);
	sVFSNode *node = vfs_node_get(p->threadDir);
	vfs_node_destroyNow(node->parent);
	vfs_fsmsgs_removeProc(pid);
}

bool vfs_createThread(tid_t tid) {
	char *name;
	sVFSNode *n,*dir;
	const Thread *t = Thread::getById(tid);

	/* build name */
	name = (char*)Cache::alloc(12);
	if(name == NULL)
		return false;
	itoa(name,12,tid);

	/* create dir */
	n = vfs_node_get(t->getProc()->threadDir);
	dir = vfs_dir_create(KERNEL_PID,n,name);
	if(dir == NULL)
		goto errorDir;

	/* create info-node */
	dir = vfs_node_request(vfs_node_getNo(dir));
	if(!dir)
		goto errorDir;
	n = vfs_file_create(KERNEL_PID,dir,(char*)"info",vfs_info_threadReadHandler,NULL);
	if(n == NULL)
		goto errorInfo;

	/* create trace-node */
	n = vfs_file_create(KERNEL_PID,dir,(char*)"trace",vfs_info_traceReadHandler,NULL);
	if(n == NULL)
		goto errorInfo;
	vfs_node_release(dir);
	return true;

errorInfo:
	vfs_node_release(dir);
	vfs_node_destroy(dir);
errorDir:
	Cache::free(name);
	return false;
}

void vfs_removeThread(tid_t tid) {
	char name[12];
	Thread *t = Thread::getById(tid);
	sVFSNode *n,*dir;
	bool isValid;

	/* build name */
	itoa(name,sizeof(name),tid);

	/* search for thread-node and remove it */
	dir = vfs_node_get(t->getProc()->threadDir);
	n = vfs_node_openDir(dir,true,&isValid);
	if(!isValid) {
		vfs_node_closeDir(dir,true);
		return;
	}
	while(n != NULL) {
		if(strcmp(n->name,name) == 0)
			break;
		n = n->next;
	}
	vfs_node_closeDir(dir,true);
	if(n)
		vfs_node_destroyNow(n);
}

void vfs_printMsgs(OStream &os) {
	bool isValid;
	sVFSNode *drv = vfs_node_openDir(devNode,false,&isValid);
	if(isValid) {
		os.writef("Messages:\n");
		while(drv != NULL) {
			if(IS_DEVICE(drv->mode)) {
				os.pushIndent();
				vfs_device_print(os,drv);
				os.popIndent();
			}
			drv = drv->next;
		}
	}
	vfs_node_closeDir(devNode,false);
}
