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
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/debug.h>
#include <esc/signals.h>

static int myThread(void);

void main(void) {
	/*if(fork() == 0) {
		debugf("pid=%d\n",getpid());
		exit(1);
	}*/
	/*forkThread();
	fork();
	if(gettid() == 0)
		exit(2);*/
	/*if(fork() == 0 && gettid() == 3)
		exit(1);*/

	startThread(myThread);
	startThread(myThread);
	while(1) {
		/*debugf("tid=%d,pid=%d\n",gettid(),getpid());*/
	}
}

static int myThread(void) {
	u32 i;
	for(i = 0; i < 10; i++) {
		debugf("tid=%d,pid=%d\n",gettid(),getpid());
		sleep(1000);
	}
	return 0;
	/*u32 i,end = gettid() * 100;
	for(i = 0; i < end; i++)
		debugf("tid=%d,pid=%d\n",gettid(),getpid());
	return gettid();*/
}
