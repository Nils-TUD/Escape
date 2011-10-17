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

#include <esc/common.h>
#include <esc/mem.h>
#include <stdlib.h>
#include "linecontainer.h"

LineContainer::size_type LineContainer::pageSize = 0;

bool LineContainer::Region::append(size_type size,const char *line) {
	// extend region?
	if((_lines + 1) * size >= _pages * pageSize) {
		uintptr_t oldEnd = (uintptr_t)changeSize(0);
		if(oldEnd != _begin + _pages * pageSize)
			return false;
		if(!changeSize(1))
			throw bad_alloc();
		_pages++;
	}

	// copy to end
	bool end = false;
	char *addr = (char*)(_begin + _lines * size);
	for(size_type i = 0; i < size - 1; i++) {
		if(!*line)
			end = true;
		addr[i] = end ? ' ' : *line++;
	}
	addr[size - 1] = '\0';
	_lines++;
	return true;
}

char *LineContainer::Region::get(size_type size,size_type i) const {
	if(i >= _lines)
		return NULL;
	return (char*)(_begin + i * size);
}

const char *LineContainer::get(size_type index) {
	size_type i = index;
	for(std::vector<Region>::iterator it = _regions.begin(); it != _regions.end(); ++it) {
		const char *res = it->get(_lineLen,i);
		if(res)
			return res;
		i -= it->lines();
	}
	return NULL;
}

void LineContainer::append(const char *line) {
	Region &r = _regions[_regions.size() - 1];
	if(!r.append(_lineLen,line)) {
		// appending failed, so probably the heap has used changeSize to get more pages.
		// thus, we have to add a new region
		Region nr;
		nr.append(_lineLen,line);
		_regions.push_back(nr);
	}
	_lines++;
}
