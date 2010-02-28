/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <machine/serial.h>
#include <vfs/vfs.h>
#include <vfs/node.h>
#include <vfs/rw.h>
#include <printf.h>
#include <esccodes.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <log.h>

#define VID_COLS		80
#define TAB_WIDTH		4
#define BUF_SIZE		2048
#define LOG_DIR			"/system"
#define LOG_FILENAME	"log"
#define DUMMY_STDIN		"/system/stdin"

static void log_printc(char c);
static u8 log_pipePad(void);
static s32 log_write(tTid tid,tFileNo file,sVFSNode *node,const u8 *buffer,u32 offset,u32 count);
static void log_escape(const char **str);
static void log_flush(void);

/* don't use a heap here to prevent problems */
static char buf[BUF_SIZE];
static u32 bufPos;
static u32 col = 0;

static tFileNo logFile;
static bool vfsIsReady = false;
static sPrintEnv env = {
	.print = log_printc,
	.escape = log_escape,
	.pipePad = log_pipePad
};

void log_vfsIsReady(void) {
	tInodeNo inodeNo;
	sVFSNode *logNode;
	tFileNo inFile;
	tFD in,out,err;
	char *nameCpy;
	sThread *t = thread_getRunning();

	/* open log-file */
	assert(vfsn_resolvePath(LOG_DIR,&inodeNo,NULL,VFS_CREATE) == 0);
	nameCpy = strdup(LOG_FILENAME);
	assert(nameCpy != NULL);
	logNode = vfsn_createFile(KERNEL_TID,vfsn_getNode(inodeNo),nameCpy,vfsrw_readDef,log_write);
	assert(logNode != NULL);
	logFile = vfs_openFile(KERNEL_TID,VFS_WRITE,NADDR_TO_VNNO(logNode),VFS_DEV_NO);
	assert(logFile >= 0);

	/* create stdin, stdout and stderr for initloader. out and err should write to the log-file */
	/* stdin is just a dummy file. init will remove these fds before starting the shells which will
	 * create new ones (for the vterm of the shell) */
	assert(vfsn_resolvePath(DUMMY_STDIN,&inodeNo,NULL,VFS_CREATE) == 0);
	assert((inFile = vfs_openFile(t->tid,VFS_READ,inodeNo,VFS_DEV_NO)) >= 0);
	in = thread_getFreeFd();
	assert(in == 0 && thread_assocFd(in,inFile) == 0);
	out = thread_getFreeFd();
	assert(out == 1 && thread_assocFd(out,logFile) == 0);
	err = thread_getFreeFd();
	assert(err == 2 && thread_assocFd(err,logFile) == 0);

	/* now write the stuff we've saved so far to the log-file */
	vfsIsReady = true;
	log_flush();
}

void log_printf(const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	log_vprintf(fmt,ap);
	va_end(ap);
}

void log_vprintf(const char *fmt,va_list ap) {
	prf_vprintf(&env,fmt,ap);
	log_flush();
}

static void log_printc(char c) {
	if(bufPos >= BUF_SIZE)
		log_flush();
	if(bufPos < BUF_SIZE) {
		buf[bufPos++] = c;
		switch(c) {
			case '\n':
			case '\r':
				col = 0;
				break;
			case '\t':
				col = (col + (TAB_WIDTH - col % TAB_WIDTH)) % VID_COLS;
				if(col == 0)
					log_printc('\n');
				break;
			default:
				col = (col + 1) % VID_COLS;
				if(col == 0)
					log_printc('\n');
				break;
		}
	}
}

static u8 log_pipePad(void) {
	return VID_COLS - col;
}

static void log_escape(const char **str) {
	s32 n1,n2,n3;
	escc_get(str,&n1,&n2,&n3);
}

static s32 log_write(tTid tid,tFileNo file,sVFSNode *node,const u8 *buffer,u32 offset,u32 count) {
#ifdef LOGSERIAL
	char *str = (char*)buffer;
	u32 i;
	for(i = 0; i < count; i++) {
		char c = str[i];
		/* write to COM1 (some chars make no sense here) */
		if(c != '\r' && c != '\b')
			ser_out(SER_COM1,c);
	}
#endif
	return vfsrw_writeDef(tid,file,node,buffer,offset,count);
}

static void log_flush(void) {
	if(vfsIsReady && bufPos) {
		vfs_writeFile(KERNEL_TID,logFile,(u8*)buf,bufPos);
		bufPos = 0;
	}
}
