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

#include <string>
#include <map>
#include <vthrow.h>

namespace std {
	/**
	 * Access to environment-variables
	 */
	class env {
	public:
		/**
		 * @return a map of with all environment-variables and their value
		 * @throws default_error if the fetch of a value fails
		 */
		static map<string,string> list();

		/**
		 * Changes the given path to an absolute path one. That means it prepends CWD if path
		 * does not start with a '/'.
		 *
		 * @param path your possibly relative path (will be changed)
		 * @return the absolute path
		 */
		static string& absolutify(string& path);

		/**
		 * Fetches the value of the env-variable with given name
		 *
		 * @param name the name
		 * @return the value
		 * @throws default_error if the fetch fails
		 */
		static string get(const string& name);

		/**
		 * Sets <name> to <value>
		 *
		 * @param name the name
		 * @param value the new value
		 * @throws default_error if the fetch fails
		 */
		static void set(const string& name,const string& value);

	private:
		// no construction and copying
		env();
		~env();
		env(const env& e);
		env& operator =(const env& e);
	};
}
