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
#include <vfs/vfs.h>
#include <vfs/node.h>
#include <printf.h>
#include <esccodes.h>
#include <stdarg.h>
#include <assert.h>
#include <log.h>

#define VID_COLS		80
#define TAB_WIDTH		4
#define BUF_SIZE		2048
#define LOG_FILE		"/system/log"

static void log_printc(char c);
static u8 log_pipePad(void);
static void log_escape(const char **str);
static void log_flush(void);

/* don't use a heap here to prevent problems */
static char buf[BUF_SIZE];
static u32 bufPos;
static u32 col = 0;

static tFileNo file;
static bool vfsIsReady = false;
static sPrintEnv env = {
	.print = log_printc,
	.escape = log_escape,
	.pipePad = log_pipePad
};

void log_vfsIsReady(void) {
	tInodeNo inodeNo;
	bool created;
	vfsIsReady = true;
	assert(vfsn_resolvePath(LOG_FILE,&inodeNo,&created,VFS_CREATE) == 0);
	file = vfs_openFile(KERNEL_TID,VFS_WRITE,inodeNo,VFS_DEV_NO);
	assert(file >= 0);
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

static void log_flush(void) {
	if(vfsIsReady && bufPos) {
		assert(vfs_writeFile(KERNEL_TID,file,(u8*)buf,bufPos) == (s32)bufPos);
		bufPos = 0;
	}
}
