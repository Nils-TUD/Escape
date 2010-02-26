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

u32 fwrite(const void *ptr,u32 size,u32 count,tFile *file) {
	u32 total;
	u8 *bPtr = (u8*)ptr;
	sBuffer *out;
	sIOBuffer *buf = bget(file);
	/* TODO this is not correct; we should return 0 in this case and set ferror */
	if(buf == NULL)
		return IO_EOF;
	/* no out-buffer? */
	if(buf->out.fd == -1)
		return IO_EOF;

	/* TODO don't write it byte for byte */
	/* write to buffer */
	out = &(buf->out);
	total = size * count;
	while(total > 0) {
		if(bprintc(out,*bPtr++) == IO_EOF)
			return count - ((total + size - 1) / size);
		total--;
	}
	return count;
}
