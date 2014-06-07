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

#include <esc/common.h>
#include <esc/dir.h>
#include <esc/mem.h>
#include <ipc/proto/vterm.h>
#include <cp/filecopy.h>
#include <stdio.h>
#include <stdlib.h>
#include <env.h>

FileCopy::FileCopy(size_t bufsize,uint fl)
		: _bufsize(bufsize), _shmname(), _shm(), _flags(fl), _cols() {
	/* create shm file */
	int fd = pshm_create(IO_READ | IO_WRITE,0666,&_shmname);
	if(fd < 0)
		throw std::default_error("pshm_create");
	/* mmap it */
	_shm = mmap(NULL,_bufsize,0,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
	close(fd);
	if(!_shm) {
		pshm_unlink(_shmname);
		throw std::default_error("mmap");
	}

	/* get vterm-size, if we should show a progress bar */
	if(_flags & FL_PROGRESS) {
		ipc::VTerm vterm(std::env::get("TERM").c_str());
		_cols = vterm.getMode().cols;
	}
}

FileCopy::~FileCopy() {
	pshm_unlink(_shmname);
	munmap(_shm);
}

void FileCopy::printSize(size_t size) {
	if(size >= 1024 * 1024 * 1024)
		printf("%4zu GiB",size / (1024 * 1024 * 1024));
	else if(size >= 1024 * 1024)
		printf("%4zu MiB",size / (1024 * 1024));
	else if(size >= 1024)
		printf("%4zu KiB",size / 1024);
	else
		printf("%4zu B  ",size);
}

void FileCopy::showProgress(const char *src,size_t pos,size_t total,ulong steps,ulong totalSteps) {
	printf("\r%-25.25s: ",src);
	printSize(pos);
	printf(" of ");
	printSize(total);
	printf(" [");
	for(ulong i = 0; i < steps; ++i)
		putchar('#');
	for(ulong i = 0; i < totalSteps - steps; ++i)
		putchar(' ');
	putchar(']');
	fflush(stdout);
}

void FileCopy::showSimpleProgress(const char *src,size_t size) {
	ulong totalSteps = getTotalSteps();
	ulong steps = getSteps(totalSteps,size,size);
	showProgress(src,size,size,steps,totalSteps);
	putchar('\n');
	fflush(stdout);
}

bool FileCopy::copyFile(const char *src,const char *dest,bool remove) {
	sFileInfo info;
	if(stat(dest,&info) == 0 && (~_flags & FL_FORCE)) {
		handleError("skipping '%s', '%s' exists",src,dest);
		return false;
	}

	int infd = open(src,IO_READ);
	if(infd < 0) {
		handleError("open of '%s' for reading failed",src);
		return false;
	}

	int outfd = open(dest,IO_WRITE | IO_CREATE | IO_TRUNCATE);
	if(outfd < 0) {
		close(infd);
		handleError("open of '%s' for writing failed",dest);
		return false;
	}

	/* we don't care whether that is supported or not */
	if(sharefile(infd,_shm) < 0) {}
	if(sharefile(outfd,_shm) < 0) {}

	ssize_t res;
	bool success = true;
	if(_flags & FL_PROGRESS) {
		size_t total = filesize(infd);
		size_t pos = 0;
		size_t lastPos = -1;
		ulong lastSteps = -1;
		ulong totalSteps = getTotalSteps();
		while((res = read(infd,_shm,_bufsize)) > 0) {
			if(write(outfd,_shm,res) != res) {
				handleError("write to '%s' failed",dest);
				success = false;
				break;
			}

			pos += res;
			ulong steps = getSteps(totalSteps,pos,total);
			if(steps != lastSteps || pos - lastPos > 1024 * 1024) {
				showProgress(src,pos,total,steps,totalSteps);
				lastSteps = steps;
				lastPos = pos;
			}
		}
		if(res == 0) {
			putchar('\n');
			fflush(stdout);
		}
	}
	else {
		while((res = read(infd,_shm,_bufsize)) > 0) {
			if(write(outfd,_shm,res) != res) {
				handleError("write to '%s' failed",dest);
				success = false;
				break;
			}
		}
	}
	if(res < 0) {
		handleError("read from '%s' failed",src);
		success = false;
	}

	close(outfd);
	close(infd);

	if(success && remove) {
		if(unlink(src) < 0) {
			handleError("Unlinking '%s' failed",src);
			return false;
		}
	}
	return success;
}

bool FileCopy::copy(const char *src,const char *dest,bool remove) {
	char srccpy[MAX_PATH_LEN];
	char dstcpy[MAX_PATH_LEN];
	strnzcpy(srccpy,src,sizeof(srccpy));
	char *filename = basename(srccpy);
	snprintf(dstcpy,sizeof(dstcpy),"%s/%s",dest,filename);

	if(isdir(src)) {
		if(~_flags & FL_RECURSIVE) {
			handleError("omitting directory '%s'",src);
			return false;
		}

		if(mkdir(dstcpy) < 0) {
			handleError("mkdir '%s' failed",dstcpy);
			return false;
		}

		DIR *dir = opendir(src);
		if(!dir) {
			/* ignore the error here */
			if(rmdir(dstcpy) < 0) {}
			handleError("open of '%s' failed",src);
			return false;
		}

		sDirEntry e;
		bool res = true;
		while(readdir(dir,&e)) {
			if(strcmp(e.name,".") == 0 || strcmp(e.name,"..") == 0)
				continue;

			snprintf(srccpy,sizeof(srccpy),"%s/%s",src,e.name);
			if(!copy(srccpy,dstcpy,remove))
				res = false;
		}
		closedir(dir);

		if(res && remove) {
			if(rmdir(src) < 0) {
				handleError("Removing '%s' failed",src);
				return false;
			}
		}
		return res;
	}

	return copyFile(src,dstcpy,remove);
}

bool FileCopy::move(const char *src,const char *dstdir,const char *filename) {
	char dst[MAX_PATH_LEN];
	snprintf(dst,sizeof(dst),"%s/%s",dstdir,filename);

	/* source and destination-folder have to exist */
	sFileInfo srcInfo,dstDirInfo,dstInfo;
	if(stat(src,&srcInfo) < 0) {
		handleError("stat for '%s' failed",src);
		return false;
	}
	if(stat(dstdir,&dstDirInfo) < 0) {
		handleError("stat for '%s' failed",dstdir);
		return false;
	}

	int res = stat(dst,&dstInfo);
	if(res == 0) {
		if(~_flags & FL_FORCE) {
			handleError("'%s' exists. Skipping.",dst);
			return false;
		}
		/* this will fail if dst is a non-empty directory */
		if(unlink(dst) < 0) {
			handleError("Unable to unlink '%s'",dst);
			return false;
		}
	}

	/* use copy if it's on a different device */
	dev_t dstDevice = res == 0 ? dstInfo.device : dstDirInfo.device;
	if(srcInfo.device != dstDevice) {
		if(!copy(src,dstdir,true))
			return false;
	}
	/* we can't link directories */
	else if(S_ISDIR(srcInfo.mode)) {
		char subsrc[MAX_PATH_LEN];
		DIR *dir = opendir(src);
		if(!dir) {
			handleError("Opening dir '%s' failed",src);
			return false;
		}

		if(mkdir(dst) < 0) {
			closedir(dir);
			handleError("Creation of '%s' failed",dst);
			return false;
		}

		/* move all files in that directory to the destination */
		sDirEntry e;
		bool success = true;
		while(readdir(dir,&e)) {
			if(strcmp(e.name,".") == 0 || strcmp(e.name,"..") == 0)
				continue;

			snprintf(subsrc,sizeof(subsrc),"%s/%s",src,e.name);
			if(!move(subsrc,dst,e.name))
				success = false;
		}
		closedir(dir);
		if(!success)
			return false;

		if(rmdir(src) < 0) {
			handleError("Removing '%s' failed",src);
			return false;
		}
	}
	else {
		if(rename(src,dst) < 0) {
			handleError("Renaming '%s' to '%s' failed",src,dst);
			return false;
		}

		/* pretend that we've shown a progress-bar during that operation ;) */
		if(_flags & FileCopy::FL_PROGRESS)
			showSimpleProgress(src,srcInfo.size);
	}
	return true;
}
