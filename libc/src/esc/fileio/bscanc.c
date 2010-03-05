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

char bscanc(sBuffer *buf) {
	/* file */
	if(buf->type & BUF_TYPE_FILE) {
		if(buf == &((sIOBuffer*)stdin)->in)
			fflush(stdout);
		if(buf->pos >= buf->length) {
			s32 count = read(buf->fd,buf->str,buf->max);
			if(count <= 0)
				return IO_EOF;
			buf->pos = 0;
			buf->length = count;
		}
		return buf->str[buf->pos++];
	}

	/* string */
	if(buf->str[buf->pos] == '\0')
		return IO_EOF;
	return buf->str[(buf->pos)++];
}
