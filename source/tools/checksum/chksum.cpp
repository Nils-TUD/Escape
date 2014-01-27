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

#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

static unsigned char checksum(istream &in) {
	unsigned char sum = 0;
	while(!in.eof()) {
		char c = in.get();
		if(c != EOF)
			sum += c;
	}
	return sum;
}

int main(int argc,char **argv) {
	if(argc > 1) {
		for(int i = 1; i < argc; ++i) {
			ifstream in(argv[1]);
			unsigned res = checksum(in);
			cout << "checksum of '" << argv[1] << "': 0x" << hex << res << '\n';
		}
	}
	else {
		unsigned res = checksum(cin);
		cout << "checksum of stdin: 0x" << hex << res << '\n';
	}
	return EXIT_SUCCESS;
}
