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

#include <sys/common.h>
#include <stdio.h>

#include "iobuf.h"

int bputc(FILE *f,int c) {
	char cbuf = c;
	ssize_t res = bwrite(f,&cbuf,1);
	if(res < 0)
		return EOF;
	/* flush stderr on '\n' */
	if(f == stderr && c == '\n')
		RETERR(bflush(f));
	/* cast to unsigned char and back to int to ensure that chars < 0 (e.g. german umlaute) are
	 * not negative */
	return (int)(uchar)c;
}
