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

#include <sys/esccodes.h>
#include <task/filedesc.h>
#include <task/proc.h>
#include <task/thread.h>
#include <vfs/file.h>
#include <vfs/node.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <assert.h>
#include <common.h>
#include <config.h>
#include <cppsupport.h>
#include <log.h>
#include <spinlock.h>
#include <stdarg.h>
#include <string.h>
#include <video.h>

static const char *LOG_DIR			= "/sys";
static const char *DUMMY_STDIN		= "/sys/stdin";

Log Log::inst;

void Log::vfsIsReady() {
	VFSNode *dir = NULL,*stdin = NULL;
	OpenFile *inFile;
	Proc *p = Proc::getByPid(Proc::getRunning());
	const fs::User kern = fs::User::kernel();

	/* open log-file */
	sassert(VFSNode::request(kern,LOG_DIR,&dir,VFS_CREATE,FILE_DEF_MODE) == 0);
	LogFile *logNode = createObj<LogFile>(kern,dir);
	/* reserve the whole file here to prevent that we want to increase it later which might need to
	 * deadlocks if we log something at a bad place */
	logNode->reserve(MAX_LOG_SIZE);
	assert(logNode != NULL);
	VFSNode::release(dir);
	sassert(VFS::openFile(kern,0,VFS_WRITE,logNode,logNode->getNo(),VFS_DEV_NO,&inst.logFile) == 0);
	VFSNode::release(logNode);

	fs::User user(p->getUid(),p->getGid());
	user.groupCount = Groups::get(p->getPid(),user.gids,fs::MAX_GROUPS);

	/* create stdin, stdout and stderr for initloader. out and err should write to the log-file */
	/* stdin is just a dummy file. init will remove these fds before starting the shells which will
	 * create new ones (for the vterm of the shell) */
	sassert(VFSNode::request(user,DUMMY_STDIN,&stdin,VFS_CREATE,FILE_DEF_MODE) == 0);
	sassert(VFS::openFile(user,0,VFS_READ,stdin,stdin->getNo(),VFS_DEV_NO,&inFile) == 0);
	VFSNode::release(stdin);
	sassert(FileDesc::assoc(p,inFile) == 0);
	sassert(FileDesc::assoc(p,inst.logFile) == 1);
	sassert(FileDesc::assoc(p,inst.logFile) == 2);

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

ssize_t Log::LogFile::write(OpenFile *file,const void *buffer,off_t offset,size_t count) {
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
	VFSFile::write(file,buffer,offset,count);
	return count;
}

void Log::flush() {
	if(vfsReady && bufPos) {
		logFile->write(buf,bufPos);
		bufPos = 0;
	}
}
