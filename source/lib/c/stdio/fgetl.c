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
#include "iobuf.h"
#include <stdio.h>

char *fgetl(char *str,size_t max,FILE *f) {
	char *res = str;
	/* wait for one char left (\0) or a newline or error/EOF */
	while(max-- > 1) {
		int c = bgetc(f);
		if(c == EOF) {
			if(str == res)
				res = NULL;
			break;
		}
		/* don't include '\n' */
		if(c == '\n')
			break;
		*str++ = c;
	}
	*str = '\0';
	return res;
}
