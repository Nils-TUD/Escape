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
#include <esc/debug.h>
#include <esc/env.h>
#include <string.h>
#include <stdarg.h>

s32 vprinte(const char *prefix,va_list ap) {
	s32 res = 0;
	/* getEnv() may overwrite errno */
	s32 errnoBak = errno;
	char dummyBuf;
	char *msg;
	/* if we have no terminal we write it via debugf */
	if(getEnv(&dummyBuf,1,"TERM") == false) {
		vdebugf(prefix,ap);
		if(errnoBak < 0) {
			msg = strerror(errnoBak);
			debugf(": %s",msg);
		}
		debugf("\n");
		res = 0;	/* not available for debugf */
	}
	else {
		vfprintf(stderr,prefix,ap);
		if(errnoBak < 0) {
			msg = strerror(errnoBak);
			fprintf(stderr,": %s",msg);
		}
		fprintf(stderr,"\n");
	}
	return res;
}
