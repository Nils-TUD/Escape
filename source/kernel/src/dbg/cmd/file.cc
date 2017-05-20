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

#include <dbg/cmd/file.h>
#include <dbg/console.h>
#include <dbg/kb.h>
#include <dbg/lines.h>
#include <mem/kheap.h>
#include <sys/keycodes.h>
#include <task/proc.h>
#include <task/thread.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <common.h>
#include <errno.h>
#include <video.h>

static char buffer[512];

int cons_cmd_file(OStream &os,size_t argc,char **argv) {
	Lines lines;

	if(Console::isHelp(argc,argv) || argc != 2) {
		os.writef("Usage: %s <file>\n",argv[0]);
		os.writef("\tUses the current proc to be able to access the real-fs.\n");
		os.writef("\tSo, I hope, you know what you're doing ;)\n");
		return 0;
	}

	OpenFile *file = NULL;
	int res = VFS::openPath(Proc::getRunning(),VFS_READ | VFS_NOFOLLOW,0,argv[1],NULL,&file);
	if(res < 0)
		goto error;
	ssize_t count;
	while((count = file->read(buffer,sizeof(buffer))) > 0) {
		/* build lines from the read data */
		for(ssize_t i = 0; i < count; i++) {
			if(buffer[i] == '\n')
				lines.newLine();
			else
				lines.append(buffer[i]);
		}
	}
	lines.endLine();
	file->close();
	file = NULL;

	/* now display lines */
	Console::viewLines(os,&lines);
	res = 0;

error:
	/* clean up */
	if(file != NULL)
		file->close();
	return res;
}
