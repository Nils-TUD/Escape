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
#include <sys/time.h>
#include <stdio.h>

#include "../modules.h"

#define TEST_COUNT	100000

static char buf[64];

int mod_stdio(A_UNUSED int argc,A_UNUSED char **argv) {
	uint64_t start = rdtsc();
	for(int i = 0; i < TEST_COUNT; ++i)
		snprintf(buf,sizeof(buf),"%d %d\n",i,i + 1);
	uint64_t end = rdtsc();

	printf("snprintf(): %Lu cycles/call\n",(end - start) / TEST_COUNT);
	return 0;
}
