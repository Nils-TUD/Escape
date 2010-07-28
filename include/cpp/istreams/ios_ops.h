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

#ifndef IOS_OPS_H_
#define IOS_OPS_H_

#include <stddef.h>
#include <istreams/ios_base.h>

namespace std {
	// 27.4.5, manipulators:
	/**
	 * Calls str .setf(ios_base::boolalpha).
	 */
	ios_base& boolalpha(ios_base& str);
	/**
	 * Calls str .unsetf(ios_base::boolalpha).
	 */
	ios_base& noboolalpha(ios_base& str);
	/**
	 * Calls str .setf(ios_base::showbase).
	 */
	ios_base& showbase(ios_base& str);
	/**
	 * Calls str .unsetf(ios_base::showbase).
	 */
	ios_base& noshowbase(ios_base& str);
	/**
	 * Calls str .setf(ios_base::showpoint).
	 */
	ios_base& showpoint(ios_base& str);
	/**
	 * Calls str .unsetf(ios_base::showpoint).
	 */
	ios_base& noshowpoint(ios_base& str);
	/**
	 * Calls str .setf(ios_base::showpos).
	 */
	ios_base& showpos(ios_base& str);
	/**
	 * Calls str .unsetf(ios_base::showpos).
	 */
	ios_base& noshowpos(ios_base& str);
	/**
	 * Calls str .setf(ios_base::skipws).
	 */
	ios_base& skipws(ios_base& str);
	/**
	 * Calls str .unsetf(ios_base::skipws).
	 */
	ios_base& noskipws(ios_base& str);
	/**
	 * Calls str .setf(ios_base::uppercase).
	 */
	ios_base& uppercase(ios_base& str);
	/**
	 * Calls str .unsetf(ios_base::uppercase).
	 */
	ios_base& nouppercase(ios_base& str);
	/**
	 * Calls str .setf(ios_base::unitbuf).
	 */
	ios_base& unitbuf(ios_base& str);
	/**
	 * Calls str .unsetf(ios_base::unitbuf).
	 */
	ios_base& nounitbuf(ios_base& str);

	// 27.4.5.2 adjustfield:
	/**
	 * Calls str .setf(ios_base::internal, ios_base::adjustfield).
	 */
	ios_base& internal(ios_base& str);
	/**
	 * Calls str .setf(ios_base::left, ios_base::adjustfield).
	 */
	ios_base& left(ios_base& str);
	/**
	 * Calls str .setf(ios_base::right, ios_base::adjustfield).
	 */
	ios_base& right(ios_base& str);

	// 27.4.5.3 basefield:
	/**
	 * Calls str .setf(ios_base::dec, ios_base::basefield).
	 */
	ios_base& dec(ios_base& str);
	/**
	 * Calls str .setf(ios_base::hex, ios_base::basefield).
	 */
	ios_base& hex(ios_base& str);
	/**
	 * Calls str .setf(ios_base::oct, ios_base::basefield).
	 */
	ios_base& oct(ios_base& str);

	// 27.4.5.4 floatfield:
	/**
	 * Calls str .setf(ios_base::fixed, ios_base::floatfield).
	 */
	ios_base& fixed(ios_base& str);
	/**
	 * Calls str .setf(ios_base::scientific, ios_base::floatfield).
	 */
	ios_base& scientific(ios_base& str);
}

#endif /* IOS_OPS_H_ */
