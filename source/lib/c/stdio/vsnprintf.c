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
#include <stdlib.h>

#include "iobuf.h"

int vsnprintf(char *str,size_t n,const char *fmt,va_list ap) {
	int res;
	FILE sbuf;
	if(!binit(&sbuf,-1,O_WRONLY,str,0,n - 1,false))
		return EOF;

	res = vbprintf(&sbuf,fmt,ap);
	/* null-termination */
	str[sbuf.out.pos] = '\0';
	return res;
}
