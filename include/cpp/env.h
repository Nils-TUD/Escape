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

#ifndef ENV_H_
#define ENV_H_

#include <string>
#include <map>
#include <stdexcept>

namespace std {
	/**
	 * Access to environment-variables
	 */
	class env {
	public:
		/**
		 * @return a map of with all environment-variables and their value
		 * @throws io_exception if the fetch of a value fails
		 */
		static map<string,string> list();

		/**
		 * Fetches the value of the env-variable with given name
		 *
		 * @param name the name
		 * @return the value
		 * @throws io_exception if the fetch fails
		 */
		static string get(const string& name);

		/**
		 * Sets <name> to <value>
		 *
		 * @param name the name
		 * @param value the new value
		 * @throws io_exception if the fetch fails
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

#endif /* ENV_H_ */
