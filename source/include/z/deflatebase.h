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

class DeflateBase {
public:
	explicit DeflateBase();

protected:
	void build_bits_base(unsigned char *bits,unsigned short *base,int delta,int first);

	/* extra bits and base tables for length codes */
	unsigned char length_bits[30];
	unsigned short length_base[30];

	/* extra bits and base tables for distance codes */
	unsigned char dist_bits[30];
	unsigned short dist_base[30];
};

}
