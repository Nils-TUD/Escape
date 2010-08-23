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
#include <esc/messages.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cmdargs.h>
#include <file.h>
#include <vector>

#define KEYMAP_DIR		"/etc/keymaps/"

static void usage(const char *name) {
	cerr << "Usage: " << name << " [--set <name>]" << endl;
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	string kmname;
	cmdargs args(argc,argv,cmdargs::NO_FREE);
	try {
		args.parse("set=s",&kmname);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const cmdargs_error& e) {
		cerr << "Invalid arguments: " << e.what() << endl;
		usage(argv[0]);
	}

	// set keymap?
	if(!kmname.empty()) {
		string path = KEYMAP_DIR;
		sMsg msg;
		tFD fd = open("/dev/kmmanager",IO_READ | IO_WRITE);
		if(fd < 0)
			error("Unable to open keymap-manager");
		path += kmname;
		if(sendMsgData(fd,MSG_KM_SET,path.c_str(),path.size() + 1) < 0 ||
				RETRY(receive(fd,NULL,&msg,sizeof(msg))) < 0 || (s32)msg.args.arg1 < 0)
			cerr << "Setting the keymap '" << kmname << "' failed" << endl;
		else
			cout << "Successfully changed keymap to '" << kmname << "'" << endl;
		close(fd);
	}
	// list all keymaps
	else {
		file f(KEYMAP_DIR);
		vector<sDirEntry> files = f.list_files(false);
		for(vector<sDirEntry>::const_iterator it = files.begin(); it != files.end(); ++it)
			cout << it->name << endl;
	}
	return EXIT_SUCCESS;
}
