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

#include <sys/common.h>
#include <sys/stat.h>
#include <esc/stream/std.h>
#include <getopt.h>

static void usage(const char *name) {
	esc::serr << "Usage: " << name << " [-c] [-s <size>] <file>...\n";
	esc::serr << "  -c: do not create any files\n";
	esc::serr << "  -s: truncate the files to <size> bytes\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	int no_create = false;
	size_t size = 0;

	int opt;
	while((opt = getopt(argc,argv,"cs:")) != -1) {
		switch(opt) {
			case 'c': no_create = true; break;
			case 's': size = getopt_tosize(optarg); break;
			default:
				usage(argv[0]);
		}
	}
	if(optind >= argc)
		usage(argv[0]);

	int openargs = no_create ? O_WRONLY : O_WRONLY | O_CREAT;
	for(int i = optind; i < argc; ++i) {
		int fd = open(argv[i],openargs,FILE_DEF_MODE);
		if(fd < 0)
			errmsg("Unable to open '" << argv[i] << "' for writing");
		else {
			if(ftruncate(fd,size) != 0)
				errmsg("Unable to truncate '" << argv[i] << "'");
			close(fd);
		}
	}
	return 0;
}
