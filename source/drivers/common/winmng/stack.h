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

#include <list>

class Stack {
	Stack();

public:
	/**
	 * Adds the given window.
	 *
	 * @param id the window id
	 */
	static void add(gwinid_t id);

	/**
	 * Removes the given window.
	 *
	 * @param id the window id
	 */
	static void remove(gwinid_t id);

	/**
	 * Makes given window the active window.
	 *
	 * @param id the window id
	 */
	static void activate(gwinid_t id);

	/**
	 * Starts a new window sequence
	 */
	static void start();

	/**
	 * Stops the current window sequence
	 */
	static void stop();

	/**
	 * Activates the previous window in the current window sequence
	 */
	static void prev();

	/**
	 * Activates the next window in the current window sequence
	 */
	static void next();

private:
	static std::list<gwinid_t> stack;
	static std::list<gwinid_t>::iterator pos;
	static bool switched;
};
