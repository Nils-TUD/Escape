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

#include <task/filedesc.h>
#include <task/proc.h>
#include <task/thread.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <common.h>
#include <errno.h>
#include <spinlock.h>
#include <string.h>
#include <video.h>

bool FileDesc::isValid(Proc *p,int fd) {
	return fd >= 0 && (size_t)fd < p->fileDescsSize;
}

int FileDesc::init(Proc *p) {
	p->fileDescs = (OpenFile**)Cache::calloc(INIT_FD_COUNT,sizeof(OpenFile*));
	if(p->fileDescs == NULL)
		return -ENOMEM;
	p->fileDescsSize = INIT_FD_COUNT;
	return 0;
}

OpenFile *FileDesc::request(Proc *p,int fd) {
	if(EXPECT_FALSE(!isValid(p,fd)))
		return NULL;

	p->lock(PLOCK_FDS);
	OpenFile *file = p->fileDescs[fd];
	if(EXPECT_TRUE(file != NULL))
		file->incUsages();
	p->unlock(PLOCK_FDS);
	return file;
}

void FileDesc::release(OpenFile *file) {
	file->decUsages();
}

int FileDesc::clone(Proc *p) {
	Proc *cur = Thread::getRunning()->getProc();
	/* don't lock p, because its currently created; thus it can't access its file-descriptors */
	cur->lock(PLOCK_FDS);
	p->fileDescs = (OpenFile**)Cache::alloc(sizeof(OpenFile*) * cur->fileDescsSize);
	if(!p->fileDescs) {
		cur->unlock(PLOCK_FDS);
		return -ENOMEM;
	}
	p->fileDescsSize = cur->fileDescsSize;
	for(size_t i = 0; i < cur->fileDescsSize; i++) {
		p->fileDescs[i] = cur->fileDescs[i];
		if(p->fileDescs[i] != NULL)
			p->fileDescs[i]->incRefs();
	}
	cur->unlock(PLOCK_FDS);
	return 0;
}

void FileDesc::destroy(Proc *p) {
	p->lock(PLOCK_FDS);
	for(size_t i = 0; i < p->fileDescsSize; i++) {
		if(p->fileDescs[i] != NULL) {
			p->fileDescs[i]->incUsages();
			if(!p->fileDescs[i]->close(p->getPid()))
				p->fileDescs[i]->decUsages();
			p->fileDescs[i] = NULL;
		}
	}
	Cache::free(p->fileDescs);
	p->fileDescsSize = 0;
	p->unlock(PLOCK_FDS);
}

int FileDesc::assoc(Proc *p,OpenFile *file) {
	p->lock(PLOCK_FDS);
	int fd = doAssoc(p,file);
	p->unlock(PLOCK_FDS);
	return fd;
}

int FileDesc::doAssoc(Proc *p,OpenFile *file) {
	OpenFile **fds = p->fileDescs;
	for(size_t i = 0; i < p->fileDescsSize; i++) {
		if(fds[i] == NULL) {
			p->fileDescs[i] = file;
			return i;
		}
	}

	if(p->fileDescsSize == MAX_FD_COUNT)
		return -EMFILE;
	fds = (OpenFile**)Cache::realloc(p->fileDescs,p->fileDescsSize * sizeof(OpenFile*) * 2);
	if(!fds)
		return -ENOMEM;
	memclear(fds + p->fileDescsSize,p->fileDescsSize * sizeof(OpenFile*));
	int fd = p->fileDescsSize;
	fds[fd] = file;
	p->fileDescsSize *= 2;
	p->fileDescs = fds;
	return fd;
}

int FileDesc::dup(int fd) {
	int nfd = -EBADF;
	Proc *p = Thread::getRunning()->getProc();
	if(EXPECT_FALSE(!isValid(p,fd)))
		return -EBADF;

	p->lock(PLOCK_FDS);
	OpenFile *f = p->fileDescs[fd];
	nfd = doAssoc(p,f);
	if(EXPECT_TRUE(nfd >= 0))
		f->incRefs();
	p->unlock(PLOCK_FDS);
	return nfd;
}

int FileDesc::redirect(int src,int dst) {
	int err = -EBADF;
	Proc *p = Thread::getRunning()->getProc();

	/* check fds */
	if(EXPECT_FALSE(!isValid(p,src) || !isValid(p,dst)))
		return -EBADF;

	p->lock(PLOCK_FDS);
	OpenFile *fSrc = p->fileDescs[src];
	OpenFile *fDst = p->fileDescs[dst];
	if(EXPECT_TRUE(fDst != NULL)) {
		fDst->incRefs();
		if(fSrc) {
			/* we have to close the source because no one else will do it anymore... */
			fSrc->close(p->getPid());
		}
		/* now redirect src to dst */
		p->fileDescs[src] = fDst;
		err = 0;
	}
	p->unlock(PLOCK_FDS);
	return err;
}

OpenFile *FileDesc::unassoc(Proc *p,int fd) {
	if(EXPECT_FALSE(!isValid(p,fd)))
		return NULL;

	p->lock(PLOCK_FDS);
	OpenFile *file = p->fileDescs[fd];
	if(EXPECT_TRUE(file != NULL))
		p->fileDescs[fd] = NULL;
	p->unlock(PLOCK_FDS);
	return file;
}

void FileDesc::print(OStream &os,const Proc *p) {
	os.writef("File descriptors (current max=%zu):\n",p->fileDescsSize);
	for(size_t i = 0; i < p->fileDescsSize; i++) {
		if(p->fileDescs[i] != NULL) {
			os.writef("\t%-2d: ",i);
			p->fileDescs[i]->print(os);
			os.writef("\n");
		}
	}
}
