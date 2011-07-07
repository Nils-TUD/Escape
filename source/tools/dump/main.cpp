/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <cstdlib>
#include <iostream>
#include <fstream>

#define BYTES_PER_LINE	8

using namespace std;

int main(int argc,char *argv[]) {
	int c,pos;
	ifstream in;
	if(argc != 2) {
		cerr << "Usage: " << argv[0] << " <infile>" << endl;
		return EXIT_FAILURE;
	}

	in.open(argv[1],ios::in | ios::binary);
	if(!in) {
		cerr << "Unable to open " << argv[1] << " for reading" << endl;
		return EXIT_FAILURE;
	}

	pos = 0;
	while(!in.eof()) {
		c = in.get();
		printf("0x%02x",c & 0xFF);
		if(!in.eof())
			printf(", ");
		if(pos % BYTES_PER_LINE == BYTES_PER_LINE - 1)
			printf("\n");
		pos++;
	}
	if(pos % BYTES_PER_LINE != 0)
		printf("\n");

	in.close();

	return EXIT_SUCCESS;
}
