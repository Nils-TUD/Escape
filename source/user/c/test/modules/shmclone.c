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

#include <esc/common.h>
#include <esc/mem.h>
#include <esc/proc.h>
#include <string.h>
#include <stdio.h>
#include "shmclone.h"

int mod_shmclone(A_UNUSED int argc,A_UNUSED char *argv[]) {
	void *p1 = createSharedMem("foo",8192);
	void *p2 = createSharedMem("bar",4096);
	strcpy(p1,"test1");
	strcpy(p2,"test2");
	if(fork() == 0) {
		printf("p1=%s\n",p1);
		printf("p2=%s\n",p2);
		exit(EXIT_SUCCESS);
	}
	/* parent has to wait; otherwise we'll destroy the shm */
	else
		waitChild(NULL);
	return 0;
}
