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
#include <sys/task/fd.h>
#include <sys/vfs/vfs.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <string.h>
#include <errno.h>

void fd_init(sProc *p) {
	memclear(p->fileDescs,MAX_FD_COUNT * sizeof(sFile*));
}

sFile *fd_request(int fd) {
	sFile *file;
	sProc *p = thread_getRunning()->proc;
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return NULL;

	spinlock_aquire(p->locks + PLOCK_FDS);
	file = p->fileDescs[fd];
	if(file != NULL) {
		vfs_incUsages(file);
		thread_addFileUsage(file);
	}
	spinlock_release(p->locks + PLOCK_FDS);
	return file;
}

void fd_release(sFile *file) {
	thread_remFileUsage(file);
	vfs_decUsages(file);
}

void fd_clone(sProc *p) {
	size_t i;
	sProc *cur = thread_getRunning()->proc;
	/* don't lock p, because its currently created; thus it can't access its file-descriptors */
	spinlock_aquire(cur->locks + PLOCK_FDS);
	for(i = 0; i < MAX_FD_COUNT; i++) {
		p->fileDescs[i] = cur->fileDescs[i];
		if(p->fileDescs[i] != NULL)
			vfs_incRefs(p->fileDescs[i]);
	}
	spinlock_release(cur->locks + PLOCK_FDS);
}

void fd_destroy(sProc *p) {
	size_t i;
	spinlock_aquire(p->locks + PLOCK_FDS);
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != NULL) {
			vfs_incUsages(p->fileDescs[i]);
			if(!vfs_closeFile(p->pid,p->fileDescs[i]))
				vfs_decUsages(p->fileDescs[i]);
			p->fileDescs[i] = NULL;
		}
	}
	spinlock_release(p->locks + PLOCK_FDS);
}

int fd_assoc(sFile *fileNo) {
	sFile *const *fds;
	int i,fd = -EMFILE;
	sProc *p = thread_getRunning()->proc;
	spinlock_aquire(p->locks + PLOCK_FDS);
	fds = p->fileDescs;
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(fds[i] == NULL) {
			fd = i;
			break;
		}
	}
	if(fd >= 0)
		p->fileDescs[fd] = fileNo;
	spinlock_release(p->locks + PLOCK_FDS);
	return fd;
}

int fd_dup(int fd) {
	sFile *f;
	sFile *const *fds;
	int i,nfd = -EBADF;
	sProc *p = thread_getRunning()->proc;
	/* check fd */
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return -EBADF;

	spinlock_aquire(p->locks + PLOCK_FDS);
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
	spinlock_release(p->locks + PLOCK_FDS);
	return nfd;
}

int fd_redirect(int src,int dst) {
	sFile *fSrc,*fDst;
	int err = -EBADF;
	sProc *p = thread_getRunning()->proc;

	/* check fds */
	if(src < 0 || src >= MAX_FD_COUNT || dst < 0 || dst >= MAX_FD_COUNT)
		return -EBADF;

	spinlock_aquire(p->locks + PLOCK_FDS);
	fSrc = p->fileDescs[src];
	fDst = p->fileDescs[dst];
	if(fSrc != NULL && fDst != NULL) {
		vfs_incRefs(fDst);
		/* we have to close the source because no one else will do it anymore... */
		vfs_closeFile(p->pid,fSrc);
		/* now redirect src to dst */
		p->fileDescs[src] = fDst;
		err = 0;
	}
	spinlock_release(p->locks + PLOCK_FDS);
	return err;
}

sFile *fd_unassoc(int fd) {
	sFile *file;
	sProc *p = thread_getRunning()->proc;
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return NULL;

	spinlock_aquire(p->locks + PLOCK_FDS);
	file = p->fileDescs[fd];
	if(file != NULL)
		p->fileDescs[fd] = NULL;
	spinlock_release(p->locks + PLOCK_FDS);
	return file;
}

void fd_print(sProc *p) {
	size_t i;
	vid_printf("File descriptors of %d:\n",p->pid);
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != NULL) {
			vid_printf("\t%-2d: ",i);
			vfs_printFile(p->fileDescs[i]);
			vid_printf("\n");
		}
	}
}
