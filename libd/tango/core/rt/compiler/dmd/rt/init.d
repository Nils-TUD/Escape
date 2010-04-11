/**
 * $Id: init.d 610 2010-04-11 11:41:59Z nasmussen $
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

module rt.init;

typedef void (*fConstr)();

extern (C) {
	/**
	 * Start of pointer-array to constructors to call
	 */
	extern fConstr start_ctors;
	/**
	 * End of array
	 */
	extern fConstr end_ctors;

	/**
	 * We'll call this function before main() to call the constructors for global objects
	 */
	void __libd_start() {
		fConstr *constr = &start_ctors;
		while(constr < &end_ctors) {
			(*constr)();
			constr++;
		}
	}
}
