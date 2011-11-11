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
#include <sys/video.h>
#include <errno.h>

void fd_init(sProc *p) {
	size_t i;
	for(i = 0; i < MAX_FD_COUNT; i++)
		p->fileDescs[i] = -1;
}

file_t fd_request(sThread *cur,int fd) {
	file_t fileNo;
	sProc *p;
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return -EBADF;

	p = proc_request(cur,cur->proc->pid,PLOCK_FDS);
	fileNo = p->fileDescs[fd];
	if(fileNo == -1)
		fileNo = -EBADF;
	else {
		vfs_incUsages(fileNo);
		thread_addFileUsage(cur,fileNo);
	}
	proc_release(cur,p,PLOCK_FDS);
	return fileNo;
}

void fd_release(sThread *cur,file_t file) {
	thread_remFileUsage(cur,file);
	vfs_decUsages(file);
}

void fd_clone(sThread *t,sProc *p) {
	size_t i;
	sProc *cur = t->proc;
	proc_request(t,cur->pid,PLOCK_FDS);
	for(i = 0; i < MAX_FD_COUNT; i++) {
		p->fileDescs[i] = cur->fileDescs[i];
		if(p->fileDescs[i] != -1)
			vfs_incRefs(p->fileDescs[i]);
	}
	proc_release(t,cur,PLOCK_FDS);
}

void fd_destroy(sThread *t,sProc *p) {
	size_t i;
	/* release file-descriptors */
	proc_request(t,p->pid,PLOCK_FDS);
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != -1) {
			vfs_incUsages(p->fileDescs[i]);
			if(!vfs_closeFile(p->pid,p->fileDescs[i]))
				vfs_decUsages(p->fileDescs[i]);
			p->fileDescs[i] = -1;
		}
	}
	proc_release(t,p,PLOCK_FDS);
}

int fd_assoc(file_t fileNo) {
	sThread *t = thread_getRunning();
	sProc *p = proc_request(t,t->proc->pid,PLOCK_FDS);
	const file_t *fds = p->fileDescs;
	int i,fd = -EMFILE;
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(fds[i] == -1) {
			fd = i;
			break;
		}
	}
	if(fd >= 0)
		p->fileDescs[fd] = fileNo;
	proc_release(t,p,PLOCK_FDS);
	return fd;
}

int fd_dup(int fd) {
	file_t f;
	int i,nfd = -EBADF;
	sThread *t = thread_getRunning();
	sProc *p = proc_request(t,t->proc->pid,PLOCK_FDS);
	const file_t *fds = p->fileDescs;
	/* check fd */
	if(fd < 0 || fd >= MAX_FD_COUNT) {
		proc_release(t,p,PLOCK_FDS);
		return -EBADF;
	}

	f = p->fileDescs[fd];
	if(f >= 0) {
		nfd = -EMFILE;
		for(i = 0; i < MAX_FD_COUNT; i++) {
			if(fds[i] == -1) {
				/* increase references */
				nfd = i;
				vfs_incRefs(f);
				p->fileDescs[nfd] = f;
				break;
			}
		}
	}
	proc_release(t,p,PLOCK_FDS);
	return nfd;
}

int fd_redirect(int src,int dst) {
	file_t fSrc,fDst;
	int err = -EBADF;
	sProc *p;
	sThread *t = thread_getRunning();

	/* check fds */
	if(src < 0 || src >= MAX_FD_COUNT || dst < 0 || dst >= MAX_FD_COUNT)
		return -EBADF;

	p = proc_request(t,t->proc->pid,PLOCK_FDS);
	fSrc = p->fileDescs[src];
	fDst = p->fileDescs[dst];
	if(fSrc >= 0 && fDst >= 0) {
		vfs_incRefs(fDst);
		/* we have to close the source because no one else will do it anymore... */
		vfs_closeFile(p->pid,fSrc);
		/* now redirect src to dst */
		p->fileDescs[src] = fDst;
		err = 0;
	}
	proc_release(t,p,PLOCK_FDS);
	return err;
}

file_t fd_unassoc(int fd) {
	file_t fileNo;
	sProc *p;
	sThread *t = thread_getRunning();
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return -EBADF;

	p = proc_request(t,t->proc->pid,PLOCK_FDS);
	fileNo = p->fileDescs[fd];
	if(fileNo >= 0)
		p->fileDescs[fd] = -1;
	else
		fileNo = -EBADF;
	proc_release(t,p,PLOCK_FDS);
	return fileNo;
}

void fd_print(sProc *p) {
	size_t i;
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != -1) {
			vid_printf("\t\t%-2d: ",i);
			vfs_printFile(p->fileDescs[i]);
			vid_printf("\n");
		}
	}
}
