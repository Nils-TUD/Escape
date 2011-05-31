/**
 * $Id: memcpy.c 849 2010-10-04 11:04:51Z nasmussen $
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

#include <stddef.h>
#include <string.h>

void *memcpy(void *dest,const void *src,size_t len) {
	uint mod,*wdest,*wsrc;
	uchar *bdest = (uchar*)dest;
	uchar *bsrc = (uchar*)src;
	/* We have to take care not to access words on none-word-boundaries. */
	/* We can speed up the copy by copying words if both can be brought to a word-boundary */
	if((mod = ((uint)bdest % sizeof(uint))) == ((uint)bsrc % sizeof(uint))) {
		/* first, bring both on a word-boundary */
		if(mod) {
			while(len > 0 && mod++ < sizeof(uint)) {
				*bdest++ = *bsrc++;
				len--;
			}
		}
		/* now copy words as long as its possible */
		wdest = (uint*)bdest;
		wsrc = (uint*)bsrc;
		while(len >= sizeof(uint)) {
			*wdest++ = *wsrc++;
			len -= sizeof(uint);
		}
		/* copy the remaining bytes */
		bdest = (uchar*)wdest;
		bsrc = (uchar*)wsrc;
		while(len-- > 0)
			*bdest++ = *bsrc++;
	}
	else {
		/* default version */
		while(len-- > 0)
			*bdest++ = *bsrc++;
	}
	return dest;
}

