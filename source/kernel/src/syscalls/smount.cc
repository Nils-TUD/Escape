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

#include <sys/stat.h>
#include <task/filedesc.h>
#include <task/proc.h>
#include <vfs/ms.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <common.h>
#include <stdlib.h>
#include <syscalls.h>

static int getMS(Proc *p,int ms,OpenFile **msfile,VFSMS **msobj,uint perm) {
	*msfile = FileDesc::request(p,ms);
	if(*msfile == NULL)
		return -EBADF;

	/* <ms> has to be a mountspace */
	if((*msfile)->getDev() != VFS_DEV_NO || !S_ISMS((*msfile)->getNode()->getMode())) {
		FileDesc::release(*msfile);
		return -EINVAL;
	}

	*msobj = static_cast<VFSMS*>((*msfile)->getNode());

	int res;
	if((res = VFS::hasAccess(p->getPid(),*msobj,perm)) < 0) {
		FileDesc::release(*msfile);
		return res;
	}
	return 0;
}

int Syscalls::mount(Thread *t,IntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int ms = SYSC_ARG1(stack);
	int fs = SYSC_ARG2(stack);
	const char *path = (const char*)SYSC_ARG3(stack);
	uint flags = (uint)SYSC_ARG4(stack);
	Proc *p = t->getProc();
	VFSMS *msobj;
	OpenFile *fsfile,*msfile;
	int res;

	if(EXPECT_FALSE(!copyPath(abspath,sizeof(abspath),path)))
		SYSC_ERROR(stack,-EFAULT);

	/* get fs file */
	fsfile = FileDesc::request(p,fs);
	if(EXPECT_FALSE(fsfile == NULL)) {
		res = -EBADF;
		goto errPath;
	}

	/* get mountspace file */
	res = getMS(p,ms,&msfile,&msobj,VFS_WRITE);
	if(res < 0)
		goto errorMs;

	/* if it's a filesystem, mount it */
	if(fsfile->getDev() == VFS_DEV_NO && IS_CHANNEL(fsfile->getNode()->getMode()) &&
			IS_FS(fsfile->getNode()->getParent()->getMode())) {
		res = msobj->mount(p,abspath,fsfile);
	}
	else {
		if((flags & (VFS_READ | VFS_WRITE | VFS_EXEC)) == 0) {
			res = -EINVAL;
			goto errorFs;
		}

		/* otherwise, remount the directory */
		struct stat info;
		if((res = fsfile->fstat(p->getPid(),&info)) < 0)
			goto errorFs;
		if(!S_ISDIR(info.st_mode)) {
			res = -ENOTDIR;
			goto errorFs;
		}

		res = msobj->remount(p,abspath,fsfile,flags);
	}

errorFs:
	FileDesc::release(msfile);
errorMs:
	FileDesc::release(fsfile);
errPath:
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RESULT(stack,res);
}

int Syscalls::unmount(Thread *t,IntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int ms = SYSC_ARG1(stack);
	const char *path = (const char*)SYSC_ARG2(stack);

	if(EXPECT_FALSE(!copyPath(abspath,sizeof(abspath),path)))
		SYSC_ERROR(stack,-EFAULT);

	OpenFile *msfile;
	VFSMS *msobj;
	int res = getMS(t->getProc(),ms,&msfile,&msobj,VFS_WRITE);
	if(res < 0)
		SYSC_ERROR(stack,res);

	res = msobj->unmount(t->getProc(),abspath);
	FileDesc::release(msfile);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RESULT(stack,res);
}

int Syscalls::clonems(Thread *t,IntrptStackFrame *stack) {
	char namecpy[64];
	int ms = SYSC_ARG1(stack);
	const char *name = (const char*)SYSC_ARG2(stack);
	int res;

	if(!isStrInUserSpace(name,NULL))
		SYSC_ERROR(stack,-EINVAL);
	strncpy(namecpy,name,sizeof(namecpy));

	OpenFile *msfile;
	VFSMS *msobj;
	if((res = getMS(t->getProc(),ms,&msfile,&msobj,VFS_READ)) < 0)
		SYSC_ERROR(stack,res);

	res = VFS::cloneMS(t->getProc(),msobj,namecpy);
	FileDesc::release(msfile);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RESULT(stack,res);
}

int Syscalls::joinms(Thread *t,IntrptStackFrame *stack) {
	int ms = SYSC_ARG1(stack);

	OpenFile *msfile;
	VFSMS *msobj;
	int res = getMS(t->getProc(),ms,&msfile,&msobj,VFS_READ);
	if(res < 0)
		SYSC_ERROR(stack,res);

	msobj->join(t->getProc());
	FileDesc::release(msfile);
	SYSC_RESULT(stack,res);
}
