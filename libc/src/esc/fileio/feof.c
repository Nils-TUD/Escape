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

s32 feof(tFile *file) {
	sIOBuffer *buf = bget(file);
	if(buf == NULL)
		return IO_EOF;

	/* this is also ok for reading+writing because in this case we're @EOF if we've read the whole
	 * file */
	if(buf->in.fd != -1) {
		/* when using the in-buffer we're not at EOF if there's something buffered */
		if(buf->in.pos < buf->in.length)
			return false;
		/* otherwise ask the kernel */
		return eof(buf->in.fd);
	}
	/* always ask the kernel for writing-only */
	return eof(buf->out.fd);
}
