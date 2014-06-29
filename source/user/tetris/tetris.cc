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

#include <esc/common.h>
#include <cmdargs.h>
#include <stdlib.h>

#include "game.h"

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-f <cols>x<rows>] [-s <size>] [--nosound]\n",name);
	fprintf(stderr,"    <cols>x<rows> selects the screen mode, which defines the rows in the game\n");
	fprintf(stderr,"    <size> may be 1 for small or 2 for large stones\n");
	fprintf(stderr,"    --nosound disables the use of the PC speaker\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"In-game key-combinations:\n");
	fprintf(stderr,"    left/right/down: move stone\n");
	fprintf(stderr,"    up: rotate stone\n");
	fprintf(stderr,"    space: drop stone\n");
	fprintf(stderr,"    p: toggle pause\n");
	fprintf(stderr,"    r: restart\n");
	fprintf(stderr,"    q: quit\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	char *fieldsize = NULL;
	int stonesize = 2;
	int nosound = false;

	std::cmdargs args(argc,argv,std::cmdargs::NO_FREE);
	try {
		args.parse("f=s s=d nosound",&fieldsize,&stonesize,&nosound);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const std::cmdargs_error& e) {
		std::cerr << "Invalid arguments: " << e.what() << '\n';
		usage(argv[0]);
	}

	int cols = 100;
	int rows = 37;
	if(fieldsize) {
		if(sscanf(fieldsize,"%dx%d",&cols,&rows) != 2)
			usage(argv[0]);
	}

	Game::start(cols,rows,stonesize,!nosound);
	Game::stop();
	return 0;
}
