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

#pragma once

#include <sys/common.h>

namespace z {

/**
 * Computes the Cyclic Redundancy Check.
 */
class CRC32 {
public:
	typedef uint32_t type;

	explicit CRC32();

	/**
	 * Computes the CRC32 of <buf>[0] .. <buf>[<len>-1].
	 *
	 * @param buf the buffer
	 * @param len the length
	 * @return the CRC32
	 */
	type get(const void *buf,size_t len) {
		return update(0,buf,len);
	}

	/**
	 * Updates a running CRC with the bytes <buf>[0] .. <buf>[<len>-1]. Example:
	 * CRC32 c;
	 * type crc = 0;
	 * while(read_buffer(buffer,length) != EOF) {
	 *   crc = c.update(crc,buffer,length);
	 * }
	 *
	 * @param crc the current CRC
	 * @param len the length
	 * @return the updated CRC32
	 */
	type update(type crc,const void *buf,size_t len);

private:
	type _table[256];
};

}
