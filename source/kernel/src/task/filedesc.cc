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
#include <sys/task/thread.h>
#include <sys/task/filedesc.h>
#include <sys/vfs/vfs.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <string.h>
#include <errno.h>

void FileDesc::init(Proc *p) {
	memclear(p->fileDescs,MAX_FD_COUNT * sizeof(sFile*));
}

sFile *FileDesc::request(int fd) {
	sFile *file;
	Proc *p = Thread::getRunning()->getProc();
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return NULL;

	p->lock(PLOCK_FDS);
	file = p->fileDescs[fd];
	if(file != NULL) {
		vfs_incUsages(file);
		Thread::addFileUsage(file);
	}
	p->unlock(PLOCK_FDS);
	return file;
}

void FileDesc::release(sFile *file) {
	Thread::remFileUsage(file);
	vfs_decUsages(file);
}

void FileDesc::clone(Proc *p) {
	size_t i;
	Proc *cur = Thread::getRunning()->getProc();
	/* don't lock p, because its currently created; thus it can't access its file-descriptors */
	cur->lock(PLOCK_FDS);
	for(i = 0; i < MAX_FD_COUNT; i++) {
		p->fileDescs[i] = cur->fileDescs[i];
		if(p->fileDescs[i] != NULL)
			vfs_incRefs(p->fileDescs[i]);
	}
	cur->unlock(PLOCK_FDS);
}

void FileDesc::destroy(Proc *p) {
	size_t i;
	p->lock(PLOCK_FDS);
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != NULL) {
			vfs_incUsages(p->fileDescs[i]);
			if(!vfs_closeFile(p->getPid(),p->fileDescs[i]))
				vfs_decUsages(p->fileDescs[i]);
			p->fileDescs[i] = NULL;
		}
	}
	p->unlock(PLOCK_FDS);
}

int FileDesc::assoc(sFile *fileNo) {
	sFile *const *fds;
	int i,fd = -EMFILE;
	Proc *p = Thread::getRunning()->getProc();
	p->lock(PLOCK_FDS);
	fds = p->fileDescs;
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(fds[i] == NULL) {
			fd = i;
			break;
		}
	}
	if(fd >= 0)
		p->fileDescs[fd] = fileNo;
	p->unlock(PLOCK_FDS);
	return fd;
}

int FileDesc::dup(int fd) {
	sFile *f;
	sFile *const *fds;
	int i,nfd = -EBADF;
	Proc *p = Thread::getRunning()->getProc();
	/* check fd */
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return -EBADF;

	p->lock(PLOCK_FDS);
	fds = p->fileDescs;
	f = p->fileDescs[fd];
	if(f != NULL) {
		nfd = -EMFILE;
		for(i = 0; i < MAX_FD_COUNT; i++) {
			if(fds[i] == NULL) {
				/* increase references */
				nfd = i;
				vfs_incRefs(f);
				p->fileDescs[nfd] = f;
				break;
			}
		}
	}
	p->unlock(PLOCK_FDS);
	return nfd;
}

int FileDesc::redirect(int src,int dst) {
	sFile *fSrc,*fDst;
	int err = -EBADF;
	Proc *p = Thread::getRunning()->getProc();

	/* check fds */
	if(src < 0 || src >= MAX_FD_COUNT || dst < 0 || dst >= MAX_FD_COUNT)
		return -EBADF;

	p->lock(PLOCK_FDS);
	fSrc = p->fileDescs[src];
	fDst = p->fileDescs[dst];
	if(fSrc != NULL && fDst != NULL) {
		vfs_incRefs(fDst);
		/* we have to close the source because no one else will do it anymore... */
		vfs_closeFile(p->getPid(),fSrc);
		/* now redirect src to dst */
		p->fileDescs[src] = fDst;
		err = 0;
	}
	p->unlock(PLOCK_FDS);
	return err;
}

sFile *FileDesc::unassoc(int fd) {
	sFile *file;
	Proc *p = Thread::getRunning()->getProc();
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return NULL;

	p->lock(PLOCK_FDS);
	file = p->fileDescs[fd];
	if(file != NULL)
		p->fileDescs[fd] = NULL;
	p->unlock(PLOCK_FDS);
	return file;
}

void FileDesc::print(Proc *p) {
	size_t i;
	Video::printf("File descriptors of %d:\n",p->getPid());
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != NULL) {
			Video::printf("\t%-2d: ",i);
			vfs_printFile(p->fileDescs[i]);
			Video::printf("\n");
		}
	}
}
