/**
 * $Id: edmain.c 1082 2011-10-17 20:43:22Z nasmussen $
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
#include <esc/io.h>
#include <stdio.h>
#include <string.h>

int main(int argc,char *argv[]) {
	int i = 1;
	bool nl = true;
	if(argc > 1 && strcmp(argv[i],"-n") == 0) {
		nl = false;
		i++;
	}
	for(; i < argc; i++) {
		fputs(argv[i],stdout);
		if(i < argc - 1)
			putchar(' ');
	}
	if(nl)
		putchar('\n');
	return EXIT_SUCCESS;
}
