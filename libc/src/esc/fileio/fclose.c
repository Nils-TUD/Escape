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
#include <esc/heap.h>
#include <sllist.h>
#include "fileiointern.h"

s32 fclose(tFile *file) {
	sIOBuffer *buf = bget(file);
	if(buf == NULL)
		return IO_EOF;

	/* flush the buffer */
	fflush(file);

	/* close file */
	if(buf->in.fd >= 0)
		close(buf->in.fd);
	if(buf->out.fd >= 0)
		close(buf->out.fd);

	/* remove and free buffer */
	if(bufList != NULL)
		sll_removeFirst(bufList,buf);
	free(buf->in.str);
	free(buf->out.str);
	free(buf);
	return 0;
}
