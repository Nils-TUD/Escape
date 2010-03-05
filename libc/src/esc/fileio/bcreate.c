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
#include <esc/fileio.h>
#include <esc/io.h>
#include <esc/heap.h>
#include <sllist.h>
#include "fileiointern.h"

sSLList *bufList = NULL;

sIOBuffer *bcreate(tFD fd,u8 flags) {
	sIOBuffer *buf;

	/* create list if not already done */
	if(bufList == NULL) {
		bufList = sll_create();
		if(bufList == NULL)
			return NULL;
	}

	/* create new buffer */
	buf = (sIOBuffer*)malloc(sizeof(sIOBuffer));
	if(buf == NULL)
		return NULL;

	buf->error = 0;
	/* init in-buffer */
	buf->in.fd = -1;
	buf->in.str = NULL;
	if(flags & IO_READ) {
		buf->in.fd = fd;
		buf->in.type = BUF_TYPE_FILE;
		buf->in.pos = 0;
		buf->in.length = 0;
		buf->in.max = IN_BUFFER_SIZE;
		buf->in.str = (char*)malloc(IN_BUFFER_SIZE + 1);
		if(buf->in.str == NULL) {
			free(buf);
			return NULL;
		}
	}

	/* init out-buffer */
	buf->out.fd = -1;
	buf->out.str = NULL;
	if(flags & IO_WRITE) {
		buf->out.fd = fd;
		buf->out.type = BUF_TYPE_FILE;
		buf->out.pos = 0;
		buf->out.length = 0;
		buf->out.max = OUT_BUFFER_SIZE;
		buf->out.str = (char*)malloc(OUT_BUFFER_SIZE + 1);
		if(buf->out.str == NULL) {
			free(buf->in.str);
			free(buf);
			return NULL;
		}
	}

	sll_append(bufList,buf);
	return buf;
}
