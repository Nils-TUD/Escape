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
#include <sys/video.h>
#include <sys/printf.h>
#include <sys/log.h>
#include <sys/spinlock.h>
#include <sys/config.h>
#include <esc/esccodes.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>

#define TAB_WIDTH		4
#define LOG_DIR			"/system"
#define LOG_FILENAME	"log"
#define DUMMY_STDIN		"/system/stdin"

char Log::buf[BUF_SIZE];
size_t Log::bufPos;
uint Log::col = 0;
bool Log::logToSer = true;
sFile *Log::logFile;
bool Log::vfsReady = false;
klock_t Log::lock;
sPrintEnv Log::env(printc,escape,pipePad);

void Log::vfsIsReady() {
	inode_t inodeNo;
	sVFSNode *logNode;
	sFile *inFile;
	char *nameCpy;
	pid_t pid = Proc::getRunning();

	/* open log-file */
	assert(vfs_node_resolvePath(LOG_DIR,&inodeNo,NULL,VFS_CREATE) == 0);
	nameCpy = strdup(LOG_FILENAME);
	assert(nameCpy != NULL);
	logNode = vfs_file_create(KERNEL_PID,vfs_node_get(inodeNo),nameCpy,vfs_file_read,write);
	assert(logNode != NULL);
	assert(vfs_openFile(KERNEL_PID,VFS_WRITE,vfs_node_getNo(logNode),VFS_DEV_NO,&logFile) == 0);

	/* create stdin, stdout and stderr for initloader. out and err should write to the log-file */
	/* stdin is just a dummy file. init will remove these fds before starting the shells which will
	 * create new ones (for the vterm of the shell) */
	assert(vfs_node_resolvePath(DUMMY_STDIN,&inodeNo,NULL,VFS_CREATE) == 0);
	assert(vfs_openFile(pid,VFS_READ,inodeNo,VFS_DEV_NO,&inFile) == 0);
	assert(FileDesc::assoc(inFile) == 0);
	assert(FileDesc::assoc(logFile) == 1);
	assert(FileDesc::assoc(logFile) == 2);

	/* now write the stuff we've saved so far to the log-file */
	vfsReady = true;
	/* don't write that again to COM1 */
	logToSer = false;
	flush();
	logToSer = true;
}

void Log::printc(char c) {
	if(Config::get(Config::LOG) && !vfsIsReady)
		writeChar(c);
	if(bufPos >= BUF_SIZE)
		flush();
	if(bufPos < BUF_SIZE) {
		buf[bufPos++] = c;
		switch(c) {
			case '\n':
			case '\r':
				col = 0;
				break;
			case '\t':
				col = (col + (TAB_WIDTH - col % TAB_WIDTH)) % VID_COLS;
				if(col == 0) {
					printc(' ');
					return;
				}
				break;
			default:
				col = (col + 1) % VID_COLS;
				if(col == 0) {
					printc('\n');
					return;
				}
				break;
		}
	}
}

uchar Log::pipePad(void) {
	return VID_COLS - col;
}

void Log::escape(const char **str) {
	int n1,n2,n3;
	escc_get(str,&n1,&n2,&n3);
}

ssize_t Log::write(pid_t pid,sFile *file,sVFSNode *node,const void *buffer,
		off_t offset,size_t count) {
	if(Config::get(Config::LOG) && logToSer) {
		char *str = (char*)buffer;
		size_t i;
		for(i = 0; i < count; i++)
			Log::writeChar(str[i]);
	}
	/* ignore errors here */
	vfs_file_write(pid,file,node,buffer,offset,count);
	return count;
}

void Log::flush(void) {
	if(vfsReady && bufPos) {
		vfs_writeFile(KERNEL_PID,logFile,buf,bufPos);
		bufPos = 0;
	}
}
