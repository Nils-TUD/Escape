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
#include <sys/io.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <target> <linkName>\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	if(argc != 3 || getopt_ishelp(argc,argv))
		usage(argv[0]);

	if(link(argv[1],argv[2]) < 0)
		error("Unable to create the link '%s' to '%s'",argv[1],argv[2]);
	return EXIT_SUCCESS;
}
