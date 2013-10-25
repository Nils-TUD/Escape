/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <utility>
#include <string>
#include <vector>

class Path {
public:
	typedef std::vector<std::pair<std::string,std::string>> list_type;
	typedef list_type::const_iterator iterator;

	explicit Path(const std::string &path) : _parts(buildParts(path)) {
	}

	iterator begin() const {
		return _parts.begin();
	}
	iterator end() const {
		return _parts.end();
	}

private:
	static list_type buildParts(const std::string &path);

	list_type _parts;
};
