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

#include <esc/common.h>
#include <esc/io/console.h>
#include <esc/exceptions/cmdargs.h>
#include <esc/util/cmdargs.h>
#include <esc/dir.h>
#include <esc/io.h>
#include <stdio.h>

static void usage(const char *name) {
	cerr->writef(cerr,"Usage: %s <path> ...\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	char path[MAX_PATH_LEN];
	sCmdArgs *args;
	TRY {
		args = cmdargs_create(argc,argv,0);
		args->parse(args,"");
		if(args->isHelp)
			usage(argv[0]);
	}
	CATCH(CmdArgsException,e) {
		cerr->writef(cerr,"Invalid arguments: %s\n",e->toString(e));
		usage(argv[0]);
	}
	ENDCATCH

	sIterator it = args->getFreeArgs(args);
	while(it.hasNext(&it)) {
		const char *arg = it.next(&it);
		abspath(path,sizeof(path),arg);
		if(mkdir(path) < 0)
			cerr->writef(cerr,"Unable to create directory '%s'\n",path);
	}
	return EXIT_SUCCESS;
}
