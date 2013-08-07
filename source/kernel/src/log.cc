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
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/filedesc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/file.h>
#include <sys/vfs/openfile.h>
#include <sys/video.h>
#include <sys/log.h>
#include <sys/spinlock.h>
#include <sys/config.h>
#include <sys/cppsupport.h>
#include <esc/esccodes.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#define TAB_WIDTH		4
#define LOG_DIR			"/system"
#define DUMMY_STDIN		"/system/stdin"

Log Log::inst;

void Log::vfsIsReady() {
	VFSNode *logNode,*dir,*stdin;
	OpenFile *inFile;
	pid_t pid = Proc::getRunning();

	/* open log-file */
	assert(VFSNode::request(LOG_DIR,&dir,NULL,VFS_CREATE) == 0);
	logNode = CREATE(LogFile,KERNEL_PID,dir);
	assert(logNode != NULL);
	VFSNode::release(dir);
	assert(VFS::openFile(KERNEL_PID,VFS_WRITE,logNode,logNode->getNo(),VFS_DEV_NO,&inst.logFile) == 0);
	VFSNode::release(logNode);

	/* create stdin, stdout and stderr for initloader. out and err should write to the log-file */
	/* stdin is just a dummy file. init will remove these fds before starting the shells which will
	 * create new ones (for the vterm of the shell) */
	assert(VFSNode::request(DUMMY_STDIN,&stdin,NULL,VFS_CREATE) == 0);
	assert(VFS::openFile(pid,VFS_READ,stdin,stdin->getNo(),VFS_DEV_NO,&inFile) == 0);
	VFSNode::release(stdin);
	assert(FileDesc::assoc(inFile) == 0);
	assert(FileDesc::assoc(inst.logFile) == 1);
	assert(FileDesc::assoc(inst.logFile) == 2);

	/* now write the stuff we've saved so far to the log-file */
	inst.vfsReady = true;
	/* don't write that again to COM1 */
	inst.logToSer = false;
	inst.flush();
	inst.logToSer = true;
}

void Log::writec(char c) {
	SpinLock::acquire(&lock);
	if(c != '\0' && Config::get(Config::LOG) && !vfsReady)
		toSerial(c);
	if(c == '\0' || bufPos >= BUF_SIZE)
		flush();
	if(c != '\0' && bufPos < BUF_SIZE) {
		buf[bufPos++] = c;
		switch(c) {
			case '\n':
			case '\r':
				col = 0;
				break;
			case '\t':
				col = (col + (TAB_WIDTH - col % TAB_WIDTH)) % VID_COLS;
				if(col == 0) {
					SpinLock::release(&lock);
					writec(' ');
					return;
				}
				break;
			default:
				col = (col + 1) % VID_COLS;
				if(col == 0) {
					SpinLock::release(&lock);
					writec('\n');
					return;
				}
				break;
		}
	}
	SpinLock::release(&lock);
}

uchar Log::pipepad() const {
	return VID_COLS - col;
}

bool Log::escape(const char **str) {
	int n1,n2,n3;
	escc_get(str,&n1,&n2,&n3);
	return true;
}

ssize_t Log::LogFile::write(pid_t pid,OpenFile *file,const void *buffer,off_t offset,size_t count) {
	if(Config::get(Config::LOG) && Log::get().logToSer) {
		char *str = (char*)buffer;
		for(size_t i = 0; i < count; i++)
			toSerial(str[i]);
	}
	/* ignore errors here */
	VFSFile::write(pid,file,buffer,offset,count);
	return count;
}

void Log::flush() {
	if(vfsReady && bufPos) {
		logFile->writeFile(KERNEL_PID,buf,bufPos);
		bufPos = 0;
	}
}
