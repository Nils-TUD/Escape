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
#include <ctype.h>
#include <stdio.h>

#include "iobuf.h"

/* execute <expr> and return count if its < 0; otherwise you get the value of <expr> */
#define READERR(count,expr)		({ \
		int __err = (expr); \
		if(__err < 0) \
			return (count); \
		__err; \
	})

int vbscanf(FILE *f,const char *fmt,va_list ap) {
	char *str = NULL,c,rc = 0;
	int count = 0;
	int length;
	int flags;
	bool readFlags;
	bool discard;

	while(1) {
		/* wait for a '%' */
		while((c = *fmt++) != '%') {
			/* finished? */
			if(c == '\0')
				return count;
			rc = READERR(count,bgetc(f));
			if(rc != c)
				return count;
		}

		/* skip whitespace */
		do {
			rc = READERR(count,bgetc(f));
		}
		while(isspace(rc));
		READERR(count,bback(f));

		/* read flags */
		flags = 0;
		discard = false;
		readFlags = true;
		while(readFlags) {
			switch(*fmt) {
				case '*':
					discard = true;
					fmt++;
					break;
				case 'h':
					flags |= FFL_SHORT;
					fmt++;
					break;
				case 'l':
					flags |= FFL_LONG;
					fmt++;
					break;
				case 'L':
					flags |= FFL_LONGLONG;
					fmt++;
					break;
				default:
					readFlags = false;
					break;
			}
		}

		/* read length */
		length = 0;
		while(*fmt >= '0' && *fmt <= '9') {
			length = length * 10 + (*fmt - '0');
			fmt++;
		}
		if(length == 0)
			length = -1;

		/* determine format */
		switch(c = *fmt++) {
			/* integers */
			case 'b':
			case 'o':
			case 'x':
			case 'X':
			case 'u':
			case 'd': {
				llong val = 0;
				READERR(count,breadn(f,&val,length,c));
				/* store value */
				if(!discard) {
					void *n = va_arg(ap, void*);
					if(flags & FFL_SHORT)
						*(short*)n = val;
					else if(flags & FFL_LONG)
						*(long*)n = val;
					else if(flags & FFL_LONGLONG)
						*(llong*)n = val;
					else
						*(int*)n = val;
					count++;
				}
			}
			break;

			/* string */
			case 's':
				str = NULL;
				if(!discard)
					str = va_arg(ap, char*);
				count += READERR(count,breads(f,length,str));
				break;

			/* character */
			case 'c':
				if(!discard)
					str = va_arg(ap, char*);

				rc = READERR(count,bgetc(f));
				if(!discard) {
					*str = rc;
					count++;
				}
				break;

			/* error */
			default:
				return count;
		}
	}
}
