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
#include <sys/cmdargs.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "imgs/png.h"
#include "imgs/bmp.h"

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <file>...\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	if(isHelpCmd(argc,argv))
		usage(argv[0]);

	ImageInfo *imgs[] = {
		new PNGImageInfo,
		new BMPImageInfo,
	};

	for(int i = 1; i < argc; ++i) {
		std::cout << argv[i] << ": ";
		bool found = false;
		for(size_t x = 0; x < ARRAY_SIZE(imgs); ++x) {
			try {
				if(imgs[x]->print(argv[i])) {
					found = true;
					break;
				}
			}
			catch(const std::exception &e) {
				std::cout << argv[i] << ": " << e.what() << "\n";
			}
		}
		if(!found)
			std::cout << "no known image format\n";
	}
	return 0;
}
