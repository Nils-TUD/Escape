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

#include <sys/cmdargs.h>
#include <sys/common.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <mode> <file>...\n",name);
	exit(EXIT_FAILURE);
}

static mode_t parseMode(const char *mode) {
	return (mode_t)strtol(mode,NULL,0);
}

int main(int argc,const char **argv) {
	char *mode = NULL;
	mode_t imode;
	const char **args;

	int res = ca_parse(argc,argv,0,"=s*",&mode);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	imode = parseMode(mode);
	args = ca_getFree();
	while(*args) {
		if(chmod(*args,imode) < 0)
			printe("Unable to set mode of '%s' to %04o",*args,imode);
		args++;
	}
	return EXIT_SUCCESS;
}
