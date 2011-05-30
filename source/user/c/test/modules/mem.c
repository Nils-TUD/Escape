/**
 * $Id$
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
#include <stdlib.h>
#include <stdio.h>
#include "mem.h"

#define STEP_SIZE 1024

int mod_mem(int argc,char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);
	/* actually, the print-statement here is necessary to make the second printf work. because
	 * fileio will allocate the io-buffer lazy and if we can't use the heap anymore because its full
	 * we can't create the io-buffer. */
	printf("Allocating all mem... ^^\n");
	fflush(stdout);
	while(1) {
		if(malloc(STEP_SIZE) == NULL) {
			printf("Not enough mem :(\n");
			break;
		}
	}
	return EXIT_SUCCESS;
}
