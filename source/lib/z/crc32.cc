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

#include <z/crc32.h>

namespace z {

/* source: http://tools.ietf.org/html/rfc1952 */

CRC32::CRC32() {
	/* Make the table for a fast CRC. */
	for(size_t n = 0; n < 256; n++) {
		ulong c = n;
		for(int k = 0; k < 8; k++) {
			if(c & 1)
				c = 0xedb88320L ^ (c >> 1);
			else
				c = c >> 1;
		}
		_table[n] = c;
	}
}

ulong CRC32::update(ulong crc,const void *buf,size_t len) {
	ulong c = crc ^ 0xffffffffL;
	const char *b = reinterpret_cast<const char*>(buf);

	for(size_t n = 0; n < len; n++)
		c = _table[(c ^ b[n]) & 0xff] ^ (c >> 8);
	return c ^ 0xffffffffL;
}

}
