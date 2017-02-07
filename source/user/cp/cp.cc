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

#include <esc/filecopy.h>
#include <sys/common.h>
#include <sys/stat.h>
#include <getopt.h>
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
	uint flags = 0;

	int opt;
	while((opt = getopt(argc,argv,"rfp")) != -1) {
		switch(opt) {
			case 'r': flags |= esc::FileCopy::FL_RECURSIVE; break;
			case 'f': flags |= esc::FileCopy::FL_FORCE; break;
			case 'p': flags |= esc::FileCopy::FL_PROGRESS; break;
			default:
				usage(argv[0]);
		}
	}
	if(optind + 2 > argc)
		usage(argv[0]);

	CpFileCopy cp(flags);

	const char *first = argv[optind];
	const char *last = argv[argc - 1];
	if(!isdir(last)) {
		if(argc - optind > 2)
			error("'%s' is not a directory, but there are multiple source-files.",last);
		if(isdir(first))
			error("'%s' is not a directory, but the source '%s' is.",last,first);
		cp.copyFile(first,last);
	}
	else {
		for(int i = optind; i < argc - 1; ++i)
			cp.copy(argv[i],last);
	}
	return EXIT_SUCCESS;
}
