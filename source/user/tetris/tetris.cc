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
#include <sys/common.h>
#include <getopt.h>
#include <stdlib.h>

#include "game.h"

using namespace esc;

static void usage(const char *name) {
	serr << "Usage: " << name << " [-f <cols>x<rows>] [-s <size>] [--nosound]\n";
	serr << "    <cols>x<rows> selects the screen mode, which defines the rows in the game\n";
	serr << "    <size> may be 1 for small or 2 for large stones\n";
	serr << "    --nosound disables the use of the PC speaker\n";
	serr << "\n";
	serr << "In-game key-combinations:\n";
	serr << "    left/right/down: move stone\n";
	serr << "    up: rotate stone\n";
	serr << "    space: drop stone\n";
	serr << "    p: toggle pause\n";
	serr << "    r: restart\n";
	serr << "    q: quit\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	char *fieldsize = NULL;
	int stonesize = 2;
	int nosound = false;

	int opt;
	const struct option longopts[] = {
		{"nosound",		no_argument,	0,	'n'},
		{0, 0, 0, 0},
	};
	while((opt = getopt_long(argc,argv,"f:s:",longopts,NULL)) != -1) {
		switch(opt) {
			case 'f': fieldsize = optarg; break;
			case 's': stonesize = atoi(optarg); break;
			default:
				usage(argv[0]);
		}
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
