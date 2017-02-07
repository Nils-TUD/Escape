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

#include <esc/stream/std.h>
#include <esc/filecopy.h>
#include <sys/common.h>
#include <sys/stat.h>
#include <dirent.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

using namespace esc;

static const size_t BUFFER_SIZE = 16 * 1024;

class MoveFileCopy : public FileCopy {
public:
	explicit MoveFileCopy(uint fl) : FileCopy(BUFFER_SIZE,fl) {
	}

	virtual void handleError(const char *fmt,...) {
		va_list ap;
		va_start(ap,fmt);
		vprinte(fmt,ap);
		va_end(ap);
	}
};

static void usage(const char *name) {
	serr << "Usage: " << name << " [-pf] <source> <dest>\n";
	serr << "Usage: " << name << " [-pf] <source>... <directory>\n";
	serr << "    -f: overwrite existing files\n";
	serr << "    -p: show a progress bar while moving\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	uint flags = FileCopy::FL_RECURSIVE;

	// parse params
	int opt;
	while((opt = getopt(argc,argv,"pf")) != -1) {
		switch(opt) {
			case 'p': flags |= FileCopy::FL_PROGRESS; break;
			case 'f': flags |= FileCopy::FL_FORCE; break;
			default:
				usage(argv[0]);
		}
	}
	if(optind + 2 > argc)
		usage(argv[0]);

	MoveFileCopy cp(flags);

	const char *last = argv[argc - 1];
	if(isdir(last)) {
		char src[MAX_PATH_LEN];
		for(int i = optind; i < argc - 1; ++i) {
			strnzcpy(src,argv[i],sizeof(src));
			char *filename = basename(src);
			cp.move(argv[i],last,filename);
		}
	}
	else {
		if(argc - optind != 2)
			usage(argv[0]);
		char dir[MAX_PATH_LEN];
		char filename[MAX_PATH_LEN];
		strnzcpy(dir,last,sizeof(dir));
		strnzcpy(filename,last,sizeof(filename));
		cp.move(argv[optind],dirname(dir),basename(filename));
	}
	return EXIT_SUCCESS;
}
