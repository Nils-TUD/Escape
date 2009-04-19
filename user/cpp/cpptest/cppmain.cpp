/**
 * $Id: ctmain.c 202 2009-04-11 16:07:24Z nasmussen $
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/fileio.h>
#include <stdlib.h>
#include <esc/vector.hpp>

class my {
public:
	my() {
		printf("Constructor for %p...\n",this);
	};
	~my() {
		printf("Destructor for %p...\n",this);
	};

	void doIt();
};

my x;
my *y = new my();

void my::doIt() {
	printf("Ich bins\n");
}

int main(int argc, char* argv[]) {
	unsigned int a = 0;
	a++;
	printf("a=%d\n",a);

	x.doIt();
	y->doIt();

	my *m = new my();
	m->doIt();
	delete m;
	delete y;
	return EXIT_SUCCESS;
}
