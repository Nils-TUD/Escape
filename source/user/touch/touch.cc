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
#include <sys/common.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <utime.h>

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-a] [-m] [-c] <file>...\n",name);
	fprintf(stderr,"  -a: only change the access time\n");
	fprintf(stderr,"  -m: only change the modification time\n");
	fprintf(stderr,"  -c: do not create a file\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	bool onlyAccess = false;
	bool onlyModify = false;
	bool noCreate = false;

	// parse params
	esc::cmdargs args(argc,argv,0);
	try {
		args.parse("a c m",&onlyAccess,&noCreate,&onlyModify);
		if(args.is_help() || args.get_free().size() == 0)
			usage(argv[0]);
	}
	catch(const esc::cmdargs_error& e) {
		fprintf(stderr,"Invalid arguments: %s\n",e.what());
		usage(argv[0]);
	}

	for(auto f = args.get_free().begin(); f != args.get_free().end(); ++f) {
		if(!noCreate) {
			int fd = open((*f)->c_str(),O_CREAT | O_WRONLY);
			if(!fd)
				printe("Unable to open file '%s'",(*f)->c_str());
			close(fd);
		}

		struct utimbuf utimes;
		utimes.actime = utimes.modtime = time(NULL);

		if(onlyAccess || onlyModify) {
			struct stat info;
			if(stat((*f)->c_str(),&info) < 0) {
				printe("Unable to stat '%s'",(*f)->c_str());
				continue;
			}

			if(onlyAccess)
				utimes.modtime = info.st_mtime;
			else
				utimes.actime = info.st_atime;
		}

		if(utime((*f)->c_str(),&utimes) < 0)
			printe("Unable to set access/modification time for '%s'",(*f)->c_str());
	}
	return 0;
}
