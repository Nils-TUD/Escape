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
#include <esc/io.h>
#include <stdio.h>

size_t fwrite(const void *ptr,size_t size,size_t count,FILE *file) {
	/* first flush the output */
	s32 res = fflush(file);
	if(file->out.fd < 0 || res < 0)
		return 0;
	res = write(file->out.fd,ptr,count * size);
	if(res < 0) {
		file->error = res;
		return 0;
	}
	return res / size;
}
