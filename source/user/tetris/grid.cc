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

#include <esc/common.h>

#include "grid.h"

void Grid::reset() {
	memset(_grid,0,rows() * cols());
}

int Grid::removeFull() {
	int count = 0;
	for(int y = rows() - 1; y >= 0; --y) {
		if(isFull(y)) {
			for(int x = 0; x < cols(); ++x)
				set(x,y,EMPTY);
			for(int yy = y - 1; yy >= 0; --yy) {
				for(int x = 0; x < cols(); ++x)
					set(x,yy + 1,get(x,yy));
			}
			count++;
			break;
		}
	}
	return count;
}

bool Grid::isFull(int y) const {
	for(int x = 0; x < cols(); ++x) {
		if(get(x,y) == EMPTY)
			return false;
	}
	return true;
}
