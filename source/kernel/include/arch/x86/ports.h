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

class Ports {
	Ports() = delete;

public:
	/**
	 * Reads the value from the I/O-Port <port>
	 *
	 * @param port the port
	 * @return the value
	 */
	template<typename T>
	static T in(unsigned port) {
		T val;
		asm volatile ("in %w1, %0" : "=a"(val) : "Nd"(port));
		return val;
	}

	/**
	 * Outputs the <val> to the I/O-Port <port>
	 *
	 * @param port the port
	 * @param val the value
	 */
	template<typename T>
	static void out(unsigned port, T val) {
		asm volatile ("out %0, %w1" : : "a"(val), "Nd"(port));
	}
};
