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
#include <sys/vfs/openfile.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <string.h>
#include <errno.h>

void FileDesc::init(Proc *p) {
	memclear(p->fileDescs,MAX_FD_COUNT * sizeof(OpenFile*));
}

OpenFile *FileDesc::request(int fd) {
	OpenFile *file;
	Proc *p = Thread::getRunning()->getProc();
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return NULL;

	p->lock(PLOCK_FDS);
	file = p->fileDescs[fd];
	if(file != NULL) {
		file->incUsages();
		Thread::addFileUsage(file);
	}
	p->unlock(PLOCK_FDS);
	return file;
}

void FileDesc::release(OpenFile *file) {
	Thread::remFileUsage(file);
	file->decUsages();
}

void FileDesc::clone(Proc *p) {
	size_t i;
	Proc *cur = Thread::getRunning()->getProc();
	/* don't lock p, because its currently created; thus it can't access its file-descriptors */
	cur->lock(PLOCK_FDS);
	for(i = 0; i < MAX_FD_COUNT; i++) {
		p->fileDescs[i] = cur->fileDescs[i];
		if(p->fileDescs[i] != NULL)
			p->fileDescs[i]->incRefs();
	}
	cur->unlock(PLOCK_FDS);
}

void FileDesc::destroy(Proc *p) {
	size_t i;
	p->lock(PLOCK_FDS);
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != NULL) {
			p->fileDescs[i]->incUsages();
			if(!p->fileDescs[i]->closeFile(p->getPid()))
				p->fileDescs[i]->decUsages();
			p->fileDescs[i] = NULL;
		}
	}
	p->unlock(PLOCK_FDS);
}

int FileDesc::assoc(OpenFile *fileNo) {
	OpenFile *const *fds;
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
	OpenFile *f;
	OpenFile *const *fds;
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
				f->incRefs();
				p->fileDescs[nfd] = f;
				break;
			}
		}
	}
	p->unlock(PLOCK_FDS);
	return nfd;
}

int FileDesc::redirect(int src,int dst) {
	OpenFile *fSrc,*fDst;
	int err = -EBADF;
	Proc *p = Thread::getRunning()->getProc();

	/* check fds */
	if(src < 0 || src >= MAX_FD_COUNT || dst < 0 || dst >= MAX_FD_COUNT)
		return -EBADF;

	p->lock(PLOCK_FDS);
	fSrc = p->fileDescs[src];
	fDst = p->fileDescs[dst];
	if(fSrc != NULL && fDst != NULL) {
		fDst->incRefs();
		/* we have to close the source because no one else will do it anymore... */
		fSrc->closeFile(p->getPid());
		/* now redirect src to dst */
		p->fileDescs[src] = fDst;
		err = 0;
	}
	p->unlock(PLOCK_FDS);
	return err;
}

OpenFile *FileDesc::unassoc(int fd) {
	OpenFile *file;
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

void FileDesc::print(OStream &os,Proc *p) {
	size_t i;
	os.writef("File descriptors of %d:\n",p->getPid());
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != NULL) {
			os.writef("\t%-2d: ",i);
			p->fileDescs[i]->print(os);
			os.writef("\n");
		}
	}
}
