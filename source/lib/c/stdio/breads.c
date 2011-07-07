/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <ctype.h>

int breads(FILE *f,size_t length,char *str) {
	int rc;
	while(length != 0) {
		rc = bgetc(f);
		if(rc == EOF)
			break;
		if(!isspace(rc)) {
			if(str)
				*str++ = rc;
			if(length > 0)
				length--;
		}
		else {
			bback(f);
			break;
		}
	}

	if(str) {
		*str = '\0';
		return 1;
	}
	return 0;
}
