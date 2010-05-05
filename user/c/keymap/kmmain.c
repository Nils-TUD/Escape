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
#include <esc/io.h>
#include <stdio.h>
#include <esc/proc.h>
#include <esc/io/file.h>
#include <esc/io/console.h>
#include <esc/exceptions/cmdargs.h>
#include <esc/util/cmdargs.h>
#include <string.h>
#include <messages.h>

#define KEYMAP_DIR		"/etc/keymaps"

static void usage(const char *name) {
	cerr->writef(cerr,"Usage: %s [--set <name>]\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,const char **argv) {
	char *kmname = NULL;
	sCmdArgs *args;

	TRY {
		args = cmdargs_create(argc,argv,CA_NO_FREE);
		args->parse(args,"set=s",&kmname);
		if(args->isHelp)
			usage(argv[0]);
	}
	CATCH(CmdArgsException,e) {
		cerr->writef(cerr,"Invalid arguments: %s\n",e->toString(e));
		usage(argv[0]);
	}
	ENDCATCH

	/* set keymap? */
	if(kmname != NULL) {
		char path[MAX_PATH_LEN];
		sMsg msg;
		u32 len;
		tFD fd = open("/dev/kmmanager",IO_READ | IO_WRITE);
		if(fd < 0)
			error("Unable to open keymap-manager");
		len = snprintf(path,sizeof(path),KEYMAP_DIR"/%s",kmname);
		if(sendMsgData(fd,MSG_KM_SET,path,len + 1) < 0 ||
				receive(fd,NULL,&msg,sizeof(msg)) < 0 || (s32)msg.args.arg1 < 0)
			cerr->writef(cerr,"Setting the keymap '%s' failed\n",kmname);
		else
			cout->writef(cout,"Successfully changed keymap to '%s'\n",kmname);
		close(fd);
	}
	/* list all keymaps */
	else {
		sDirEntry *e;
		sFile *f = file_get(KEYMAP_DIR);
		sVector *files = f->listFiles(f,false);
		vforeach(files,e)
			cout->writeln(cout,e->name);
		vec_destroy(files,true);
		f->destroy(f);
	}
	return EXIT_SUCCESS;
}
