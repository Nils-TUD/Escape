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
#include "fileiointern.h"

u32 fread(void *ptr,u32 size,u32 count,tFile *file) {
	u32 total;
	s32 res;
	u8 *bPtr = (u8*)ptr;
	sBuffer *in;
	sIOBuffer *buf = bget(file);
	if(buf == NULL)
		return IO_EOF;
	/* no in-buffer? */
	if(buf->in.fd == -1)
		return IO_EOF;

	/* first read from buffer */
	in = &(buf->in);
	total = size * count;
	while(in->pos < in->length) {
		*bPtr++ = in->str[in->pos++];
		total--;
	}

	/* TODO maybe we should use in smaller steps, if usefull? */
	/* read from file */
	res = read(in->fd,bPtr,total);
	if(res < 0)
		return count - ((total + size - 1) / size);
	/* handle EOF from vterm */
	/* TODO this is not really nice, right? */
	if(res > 0 && bPtr[0] == (u8)IO_EOF)
		return 0;
	total -= res;
	return count - ((total + size - 1) / size);
}
