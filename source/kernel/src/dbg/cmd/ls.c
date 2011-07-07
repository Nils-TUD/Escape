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
#include <sys/vfs/vfs.h>
#include <sys/dbg/console.h>
#include <sys/dbg/cmd/ls.h>
#include <sys/mem/cache.h>
#include <esc/fsinterface.h>
#include <esc/endian.h>
#include <string.h>
#include <errors.h>

#define DIRE_SIZE		(sizeof(sDirEntry) - (MAX_NAME_LEN + 1))

static int cons_cmd_ls_read(pid_t pid,file_t file,sDirEntry *e);

static sScreenBackup backup;

int cons_cmd_ls(size_t argc,char **argv) {
	sProc *p = proc_getRunning();
	sLines lines;
	sStringBuffer buf;
	file_t file;
	sDirEntry e;
	int res;
	if(argc != 2) {
		vid_printf("Usage: %s <dir>\n",argv[0]);
		vid_printf("\tUses the current proc to be able to access the real-fs.\n");
		vid_printf("\tSo, I hope, you know what you're doing ;)\n");
		return 0;
	}

	/* create lines and redirect prints */
	if((res = lines_create(&lines)) < 0)
		return res;
	file = vfs_openPath(p->pid,VFS_READ,argv[1]);
	if(file < 0) {
		res = file;
		goto errorLines;
	}
	while((res = cons_cmd_ls_read(p->pid,file,&e)) > 0) {
		buf.dynamic = true;
		buf.len = 0;
		buf.size = 0;
		buf.str = NULL;
		prf_sprintf(&buf,"%d %s",e.nodeNo,e.name);
		if(buf.str) {
			lines_appendStr(&lines,buf.str);
			cache_free(buf.str);
		}
		if((res = lines_newline(&lines)) < 0)
			goto errorFile;
	}
	vfs_closeFile(p->pid,file);
	if(res < 0)
		goto errorLines;
	lines_end(&lines);

	/* view the lines */
	vid_backup(backup.screen,&backup.row,&backup.col);
	cons_viewLines(&lines);
	lines_destroy(&lines);
	vid_restore(backup.screen,backup.row,backup.col);
	return 0;

errorFile:
	vfs_closeFile(p->pid,file);
errorLines:
	lines_destroy(&lines);
	return res;
}

static int cons_cmd_ls_read(pid_t pid,file_t file,sDirEntry *e) {
	ssize_t res;
	size_t len;
	/* default way; read the entry without name first */
	if((res = vfs_readFile(pid,file,e,DIRE_SIZE)) < 0)
		return res;
	/* EOF? */
	if(res == 0)
		return 0;

	e->nameLen = le16tocpu(e->nameLen);
	e->recLen = le16tocpu(e->recLen);
	e->nodeNo = le32tocpu(e->nodeNo);
	len = e->nameLen;
	/* ensure that the name is short enough */
	if(len >= MAX_NAME_LEN)
		return ERR_INVALID_FILE;

	/* now read the name */
	if((res = vfs_readFile(pid,file,e->name,len)) < 0)
		return res;

	/* if the record is longer, we have to skip the stuff until the next record */
	if(e->recLen - DIRE_SIZE > len) {
		len = (e->recLen - DIRE_SIZE - len);
		if((res = vfs_seek(pid,file,len,SEEK_CUR)) < 0)
			return res;
	}

	/* ensure that it is null-terminated */
	e->name[e->nameLen] = '\0';
	return 1;
}
