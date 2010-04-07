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
#include <esc/thread.h>
#include <esc/fileio.h>
#include <esc/proc.h>
#include "tls.h"

static int otherThread(int argc,char *argv[]);

__thread int t1;
__thread int t2;
__thread int t3 = 12345;

int mod_tls(int argc,char *argv[]) {
	u32 i;
	UNUSED(argc);
	UNUSED(argv);
	startThread(otherThread,NULL);
	for(i = 0; i < 4; i++) {
		t1++;
		t2++;
		printf("[%d] t1=%d, t2=%d, t3=%d\n",gettid(),t1,t2,t3);
		sleep(100);
	}
	return 0;
}

static int otherThread(int argc,char *argv[]) {
	u32 i;
	UNUSED(argc);
	UNUSED(argv);
	t1 = 4;
	t2 = 5;
	for(i = 0; i < 4; i++) {
		t1++;
		t2++;
		printf("[%d] t1=%d, t2=%d, t3=%d\n",gettid(),t1,t2,t3);
		sleep(100);
	}
	return 0;
}
