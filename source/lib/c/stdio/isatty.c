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

#include <esc/common.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

int isatty(int fd) {
	/* TODO actually, we are supposed to use the C++ IPC API, but this doesn't work here without
	 * pulling a bag of C++ dependencies into all C apps */
	int val = 0;
	ssize_t res = send(fd,MSG_VT_ISVTERM,NULL,0);
	if(res < 0)
		goto err;
	msgid_t mid = res;
	res = receive(fd,&mid,&val,sizeof(val));
	if(res < 0)
		goto err;

	/* this is not considered as an error in this case */
	/* note that we include no-exec-perm here because we assume that we can always send messages
	 * with stdin when it is connected to a vterm. */
	if(errno == ENOTSUP || errno == EACCES)
		errno = 0;

err:
	return val == 1;
}
