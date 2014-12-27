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

#include <z/deflatebase.h>

namespace z {

DeflateBase::DeflateBase() {
	/* build extra bits and base tables */
	build_bits_base(length_bits,length_base,4,3);
	build_bits_base(dist_bits,dist_base,2,1);

	/* fix a special case */
	length_bits[28] = 0;
	length_base[28] = 258;
}

/* build extra bits and base tables */
void DeflateBase::build_bits_base(unsigned char *bits,unsigned short *base,int delta,int first) {
	int i,sum;

	/* build bits table */
	for(i = 0; i < delta; ++i)
		bits[i] = 0;
	for(i = 0; i < 30 - delta; ++i)
		bits[i + delta] = i / delta;

	/* build base table */
	for(sum = first,i = 0; i < 30; ++i) {
		base[i] = sum;
		sum += 1 << bits[i];
	}
}

}
