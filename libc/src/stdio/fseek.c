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
#include <esc/exceptions/io.h>
#include <esc/io/iofilestream.h>
#include <stdio.h>

int fseek(FILE *stream,long int offset,int whence) {
	int res = 0;
	sIOStream *s = (sIOStream*)stream;
	TRY {
		if(s->in)
			res = s->in->seek(s->in,offset,whence);
		else
			res = s->out->seek(s->out,offset,whence);
	}
	CATCH(IOException,e) {
		s->_error = e->error;
		res = EOF;
	}
	ENDCATCH;
	return res;
}
