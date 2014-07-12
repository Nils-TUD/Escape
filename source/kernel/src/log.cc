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

#include <common.h>
#include <task/thread.h>
#include <task/proc.h>
#include <task/filedesc.h>
#include <vfs/vfs.h>
#include <vfs/node.h>
#include <vfs/file.h>
#include <vfs/openfile.h>
#include <video.h>
#include <log.h>
#include <spinlock.h>
#include <config.h>
#include <cppsupport.h>
#include <sys/esccodes.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#define TAB_WIDTH		4
#define LOG_DIR			"/sys"
#define DUMMY_STDIN		"/sys/stdin"

Log Log::inst;

void Log::vfsIsReady() {
	VFSNode *dir = NULL,*stdin = NULL;
	OpenFile *inFile;
	pid_t pid = Proc::getRunning();

	/* open log-file */
	sassert(VFSNode::request(LOG_DIR,NULL,&dir,NULL,VFS_CREATE,FILE_DEF_MODE) == 0);
	LogFile *logNode = createObj<LogFile>(KERNEL_PID,dir);
	/* reserve the whole file here to prevent that we want to increase it later which might need to
	 * deadlocks if we log something at a bad place */
	logNode->reserve(MAX_VFS_FILE_SIZE);
	assert(logNode != NULL);
	VFSNode::release(dir);
	sassert(VFS::openFile(KERNEL_PID,VFS_WRITE,logNode,logNode->getNo(),VFS_DEV_NO,&inst.logFile) == 0);
	VFSNode::release(logNode);

	/* create stdin, stdout and stderr for initloader. out and err should write to the log-file */
	/* stdin is just a dummy file. init will remove these fds before starting the shells which will
	 * create new ones (for the vterm of the shell) */
	sassert(VFSNode::request(DUMMY_STDIN,NULL,&stdin,NULL,VFS_CREATE,FILE_DEF_MODE) == 0);
	sassert(VFS::openFile(pid,VFS_READ,stdin,stdin->getNo(),VFS_DEV_NO,&inFile) == 0);
	VFSNode::release(stdin);
	sassert(FileDesc::assoc(Proc::getByPid(pid),inFile) == 0);
	sassert(FileDesc::assoc(Proc::getByPid(pid),inst.logFile) == 1);
	sassert(FileDesc::assoc(Proc::getByPid(pid),inst.logFile) == 2);

	/* now write the stuff we've saved so far to the log-file */
	inst.vfsReady = true;
	/* don't write that again to COM1 */
	inst.logToSer = false;
	inst.flush();
	inst.logToSer = true;
}

void Log::writec(char c) {
	LockGuard<SpinLock> guard(&lock);
	if(c != '\0' && Config::get(Config::LOG) && !vfsReady)
		toSerial(c);
	if(c == '\0' || bufPos >= BUF_SIZE)
		flush();
	if(c != '\0' && bufPos < BUF_SIZE) {
		buf[bufPos++] = c;
		if(c == '\n')
			flush();
	}
}

uchar Log::pipepad() const {
	return 0;
}

bool Log::escape(const char **str) {
	int n1,n2,n3;
	escc_get(str,&n1,&n2,&n3);
	return true;
}

ssize_t Log::LogFile::write(pid_t pid,OpenFile *file,const void *buffer,off_t offset,size_t count) {
	if(Config::get(Config::LOG) && Log::get().logToSer) {
		const bool toVGA = Config::get(Config::LOG_TO_VGA);
		char *str = (char*)buffer;
		for(size_t i = 0; i < count; i++) {
			toSerial(str[i]);
			if(toVGA)
				Video::get().writec(str[i]);
		}
	}
	/* ignore errors here */
	VFSFile::write(pid,file,buffer,offset,count);
	return count;
}

void Log::flush() {
	if(vfsReady && bufPos) {
		logFile->write(KERNEL_PID,buf,bufPos);
		bufPos = 0;
	}
}
