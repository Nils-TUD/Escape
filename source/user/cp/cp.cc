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

#include <esc/cmdargs.h>
#include <esc/filecopy.h>
#include <sys/common.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

static const size_t BUFFER_SIZE = 16 * 1024;

class CpFileCopy : public esc::FileCopy {
public:
	explicit CpFileCopy(uint fl) : FileCopy(BUFFER_SIZE,fl) {
	}

	virtual void handleError(const char *fmt,...) {
		va_list ap;
		va_start(ap,fmt);
		vprinte(fmt,ap);
		va_end(ap);
	}
};

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-rfp] <source>... <dest>\n",name);
	fprintf(stderr,"    -r: copy directories recursively\n");
	fprintf(stderr,"    -f: overwrite existing files\n");
	fprintf(stderr,"    -p: show a progress bar while copying\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	int rec = false;
	int force = false;
	int progress = false;

	// parse params
	esc::cmdargs args(argc,argv,0);
	try {
		args.parse("r f p",&rec,&force,&progress);
		if(args.is_help() || args.get_free().size() < 2)
			usage(argv[0]);
	}
	catch(const esc::cmdargs_error& e) {
		fprintf(stderr,"Invalid arguments: %s\n",e.what());
		usage(argv[0]);
	}

	uint flags = 0;
	if(rec)
		flags |= esc::FileCopy::FL_RECURSIVE;
	if(force)
		flags |= esc::FileCopy::FL_FORCE;
	if(progress)
		flags |= esc::FileCopy::FL_PROGRESS;
	CpFileCopy cp(flags);

	auto files = args.get_free();
	std::string *first = files[0];
	std::string *last = files[files.size() - 1];
	if(!isdir(last->c_str())) {
		if(files.size() > 2)
			error("'%s' is not a directory, but there are multiple source-files.",last->c_str());
		if(isdir(files[0]->c_str()))
			error("'%s' is not a directory, but the source '%s' is.",last->c_str(),first->c_str());
		cp.copyFile(first->c_str(),last->c_str());
	}
	else {
		auto dest = files.end() - 1;
		for(auto it = files.begin(); it != dest; ++it)
			cp.copy((*it)->c_str(),last->c_str());
	}
	return EXIT_SUCCESS;
}
