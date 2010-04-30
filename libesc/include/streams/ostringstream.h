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

#ifndef OSTRINGSTREAM_H_
#define OSTRINGSTREAM_H_

#include <esc/common.h>
#include <stdarg.h>
#include "outputstream.h"

typedef struct {
	s32 pos;
	s32 max;
	char *buffer;
} sOSStream;

sOStream *osstream_open(char *buffer,s32 size);
s32 osstream_seek(sOStream *s,s32 offset,u32 whence);
bool osstream_eof(sOStream *s);
void osstream_flush(sOStream *s);
void osstream_close(sOStream *s);

#endif /* OSTRINGSTREAM_H_ */
