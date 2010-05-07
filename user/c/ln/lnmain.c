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
#include <esc/io.h>
#include <esc/dir.h>
#include <stdio.h>

static void usage(const char *name) {
	cerr->writef(cerr,"Usage: %s <target> <linkName>\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	char oldPath[MAX_PATH_LEN];
	char newPath[MAX_PATH_LEN];
	char *oldp,*newp;

	TRY {
		sCmdArgs *args = cmdargs_create(argc,argv,CA_NO_FREE);
		args->parse(args,"=s =s",&oldp,&newp);
		if(args->isHelp)
			usage(argv[0]);
	}
	CATCH(CmdArgsException,e) {
		cerr->writef(cerr,"Invalid arguments: %s\n",e->toString(e));
		usage(argv[0]);
	}
	ENDCATCH

	abspath(oldPath,MAX_PATH_LEN,oldp);
	abspath(newPath,MAX_PATH_LEN,newp);
	if(link(oldPath,newPath) < 0)
		error("Unable to create the link '%s' to '%s'",newPath,oldPath);
	return EXIT_SUCCESS;
}
