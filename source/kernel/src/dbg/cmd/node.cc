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

#include <dbg/cmd/node.h>
#include <dbg/console.h>
#include <dbg/kb.h>
#include <dbg/lines.h>
#include <task/proc.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <common.h>
#include <errno.h>

int cons_cmd_node(OStream &os,size_t argc,char **argv) {
	Proc *p = Proc::getByPid(0);

	if(Console::isHelp(argc,argv) || argc != 2) {
		os.writef("Usage: %s <file>\n",argv[0]);
		os.writef("\tPrints information about the given file in the VFS\n");
		return 0;
	}

	Lines lines;
	Lines::OStream los(&lines);

	OpenFile *file = NULL;
	int res = VFS::openPath(p->getPid(),VFS_READ | VFS_NOFOLLOW,0,argv[1],NULL,&file);
	if(res < 0 || file->getDev() != VFS_DEV_NO)
		goto error;

	file->getNode()->print(los);
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
