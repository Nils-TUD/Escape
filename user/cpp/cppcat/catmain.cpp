/**
 * $Id: catmain.c 647 2010-05-05 17:27:58Z nasmussen $
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
#include <iostream>
#include <cmdargs.h>
#include <file.h>
#include <rawfile.h>
#include <stdlib.h>

using namespace std;

#define BUF_SIZE 512

static void printStream(rawfile& f);

static void usage(const char *name) {
	cerr << "Usage: " << name << " [<file> ...]" << endl;
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	// no exception here since we don't have required- or value-args
	cmdargs args(argc,argv,0);
	args.parse("");
	if(args.is_help())
		usage(argv[0]);

	if(argc < 2) {
		rawfile f(STDIN_FILENO);
		printStream(f);
	}
	else {
		const vector<string*> fargs = args.get_free();
		for(vector<string*>::const_iterator it = fargs.begin(); it != fargs.end(); ++it) {
			try {
				file f(**it);
				if(f.is_dir())
					cerr << "'" << **it << "' is a directory!" << endl;
				else {
					rawfile rw(**it,rawfile::READ);
					printStream(rw);
				}
			}
			catch(const io_exception& e) {
				cerr << "Unable to read file '" << **it << "': " << e.what() << endl;
			}
		}
	}
	return EXIT_SUCCESS;
}

static void printStream(rawfile& f) {
	s32 count;
	char buffer[BUF_SIZE];
	while((count = f.read(buffer,sizeof(char),BUF_SIZE)) > 0)
		cout.write(buffer,count);
}
