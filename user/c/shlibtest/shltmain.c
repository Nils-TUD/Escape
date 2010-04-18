/**
 * $Id: sortmain.c 578 2010-03-29 15:54:22Z nasmussen $
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
#include "shltlib.h"
#include "seclib.h"

int main(int argc,char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);
	double r1 = 10;
	double r2 = 3.5;
	double c1 = circumfenceDoubled(r1);
	double c2 = circumfenceSquared(r2);
	printf("c1=%f, c2=%f, c=%f\n",c1,c2,circumfence(r1));
	return EXIT_SUCCESS;
}
