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
#include "fileiointern.h"

s32 bscans(sBuffer *buf,char *buffer,u32 max) {
	char c = 0;
	char *start = buffer;
	/* wait for one char left (\0) or till EOF */
	while(max-- > 1 && (c = bscanc(buf)) != IO_EOF)
		*buffer++ = c;
	/* terminate */
	*buffer = '\0';
	return c == IO_EOF ? IO_EOF : (buffer - start);
}
