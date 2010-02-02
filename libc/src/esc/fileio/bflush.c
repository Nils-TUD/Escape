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

s32 bflush(sBuffer *buf) {
	s32 res = 0;
	if((buf->type & BUF_TYPE_FILE) && buf->pos > 0) {
		if(write(buf->fd,buf->str,buf->pos * sizeof(char)) < 0)
			res = IO_EOF;
		buf->pos = 0;
		/* a process switch after we've written some chars seems to be good :) */
		/*if(res >= 64)
			yield();*/
	}
	return res;
}
