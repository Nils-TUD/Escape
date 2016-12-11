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
#include <esc/cmdargs.h>
#include <esc/filecopy.h>
#include <sys/common.h>
#include <sys/stat.h>
#include <dirent.h>
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
	int force = false;
	int progress = false;

	// parse params
	cmdargs args(argc,argv,0);
	try {
		args.parse("f p",&force,&progress);
		if(args.is_help() || args.get_free().size() < 2)
			usage(argv[0]);
	}
	catch(const cmdargs_error& e) {
		errmsg("Invalid arguments: " << e.what());
		usage(argv[0]);
	}

	uint flags = FileCopy::FL_RECURSIVE;
	if(force)
		flags |= FileCopy::FL_FORCE;
	if(progress)
		flags |= FileCopy::FL_PROGRESS;
	MoveFileCopy cp(flags);

	auto files = args.get_free();
	std::string *last = files[files.size() - 1];
	if(isdir(last->c_str())) {
		char src[MAX_PATH_LEN];
		auto dest = files.end() - 1;
		for(auto it = files.begin(); it != dest; ++it) {
			strnzcpy(src,(*it)->c_str(),sizeof(src));
			char *filename = basename(src);
			cp.move((*it)->c_str(),last->c_str(),filename);
		}
	}
	else {
		if(files.size() != 2)
			usage(argv[0]);
		char dir[MAX_PATH_LEN];
		char filename[MAX_PATH_LEN];
		strnzcpy(dir,last->c_str(),sizeof(dir));
		strnzcpy(filename,last->c_str(),sizeof(filename));
		cp.move(files[0]->c_str(),dirname(dir),basename(filename));
	}
	return EXIT_SUCCESS;
}
