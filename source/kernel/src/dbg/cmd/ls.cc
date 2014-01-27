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

#include <sys/common.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/openfile.h>
#include <sys/dbg/console.h>
#include <sys/dbg/cmd/ls.h>
#include <sys/mem/cache.h>
#include <sys/ostringstream.h>
#include <esc/fsinterface.h>
#include <esc/endian.h>
#include <string.h>
#include <errno.h>

#define DIRE_SIZE		(sizeof(sDirEntry) - (MAX_NAME_LEN + 1))

static int cons_cmd_ls_read(pid_t pid,OpenFile *file,sDirEntry *e);

int cons_cmd_ls(OStream &os,size_t argc,char **argv) {
	pid_t pid = Proc::getRunning();
	Lines lines;
	if(Console::isHelp(argc,argv) || argc != 2) {
		os.writef("Usage: %s <dir>\n",argv[0]);
		os.writef("\tUses the current proc to be able to access the real-fs.\n");
		os.writef("\tSo, I hope, you know what you're doing ;)\n");
		return 0;
	}

	/* redirect prints */
	OpenFile *file;
	int res = VFS::openPath(pid,VFS_READ,0,argv[1],&file);
	if(res < 0)
		return res;
	sDirEntry e;
	while((res = cons_cmd_ls_read(pid,file,&e)) > 0) {
		OStringStream ss;
		ss.writef("%d %s",e.nodeNo,e.name);
		if(ss.getString())
			lines.appendStr(ss.getString());
		lines.newLine();
	}
	file->close(pid);
	if(res < 0)
		return res;
	lines.endLine();

	/* view the lines */
	Console::viewLines(os,&lines);
	return 0;
}

static int cons_cmd_ls_read(pid_t pid,OpenFile *file,sDirEntry *e) {
	ssize_t res;
	/* default way; read the entry without name first */
	if((res = file->read(pid,e,DIRE_SIZE)) < 0)
		return res;
	/* EOF? */
	if(res == 0)
		return 0;

	e->nameLen = le16tocpu(e->nameLen);
	e->recLen = le16tocpu(e->recLen);
	e->nodeNo = le32tocpu(e->nodeNo);
	size_t len = e->nameLen;
	/* ensure that the name is short enough */
	if(len >= MAX_NAME_LEN)
		return -ENAMETOOLONG;

	/* now read the name */
	if((res = file->read(pid,e->name,len)) < 0)
		return res;

	/* if the record is longer, we have to skip the stuff until the next record */
	if(e->recLen - DIRE_SIZE > len) {
		len = (e->recLen - DIRE_SIZE - len);
		if((res = file->seek(pid,len,SEEK_CUR)) < 0)
			return res;
	}

	/* ensure that it is null-terminated */
	e->name[e->nameLen] = '\0';
	return 1;
}
