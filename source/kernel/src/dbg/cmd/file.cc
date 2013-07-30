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
#include <sys/dbg/console.h>
#include <sys/dbg/kb.h>
#include <sys/dbg/lines.h>
#include <sys/dbg/cmd/file.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/mem/kheap.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/openfile.h>
#include <sys/video.h>
#include <esc/keycodes.h>
#include <errno.h>

static char buffer[512];

int cons_cmd_file(OStream &os,size_t argc,char **argv) {
	pid_t pid = Proc::getRunning();
	OpenFile *file = NULL;
	ssize_t i,count;
	int res;
	Lines lines;

	if(Console::isHelp(argc,argv) || argc != 2) {
		os.writef("Usage: %s <file>\n",argv[0]);
		os.writef("\tUses the current proc to be able to access the real-fs.\n");
		os.writef("\tSo, I hope, you know what you're doing ;)\n");
		return 0;
	}

	res = VFS::openPath(pid,VFS_READ,argv[1],&file);
	if(res < 0)
		goto error;
	while((count = file->readFile(pid,buffer,sizeof(buffer))) > 0) {
		/* build lines from the read data */
		for(i = 0; i < count; i++) {
			if(buffer[i] == '\n')
				lines.newLine();
			else
				lines.append(buffer[i]);
		}
	}
	lines.endLine();
	file->closeFile(pid);
	file = NULL;

	/* now display lines */
	Console::viewLines(os,&lines);
	res = 0;

error:
	/* clean up */
	if(file != NULL)
		file->closeFile(pid);
	return res;
}
