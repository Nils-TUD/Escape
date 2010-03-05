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
#include <esc/math.h>

s32 abs(s32 n) {
	return n < 0 ? -n : n;
}

s64 labs(s64 n) {
	return n < 0 ? -n : n;
}

tDiv div(s32 numerator,s32 denominator) {
	tDiv res;
	res.quot = numerator / denominator;
	res.rem = numerator % denominator;
	return res;
}

tLDiv ldiv(s64 numerator,s64 denominator) {
	tLDiv res;
	res.quot = numerator / denominator;
	res.rem = numerator % denominator;
	return res;
}
