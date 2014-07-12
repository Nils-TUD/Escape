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

#include <sys/common.h>

#include "stone.h"

bool Stone::move(int addx,int addy) {
	bool res = true;
	update(false);
	_x += addx;
	_y += addy;
	if(!free()) {
		_x -= addx;
		_y -= addy;
		res = false;
	}
	update(true);
	return res;
}

bool Stone::rotate() {
	bool res = true;
	update(false);

	bool backup[_size * _size];
	bool copy[_size * _size];
	memcpy(backup,_fields,sizeof(backup));
	for(int tx = _size - 1, y = 0; y < _size; ++y, --tx) {
		for(int x = 0; x < _size; ++x)
			copy[x * _size + tx] = get(x,y);
	}
	memcpy(_fields,copy,sizeof(copy));

	if(!free()) {
		memcpy(_fields,backup,sizeof(backup));
		res = false;
	}
	update(true);
	return res;
}

bool Stone::free() const {
	for(int x = 0; x < _size; ++x) {
		for(int y = 0; y < _size; ++y) {
			if(!get(x,y))
				continue;
			if(_x + x < 0 || _y + y < 0 || _x + x >= _grid.cols() || _y + y >= _grid.rows())
				return false;
			if(_grid.get(_x + x,_y + y) != Grid::EMPTY)
				return false;
		}
	}
	return true;
}

int Stone::mostRight() const {
	for(int x = _size - 1; x > 0; x--) {
		for(int y = 0; y < _size; ++y) {
			if(get(x,y))
				return x;
		}
	}
	return 0;
}

void Stone::update(bool filled) {
	for(int x = 0; x < _size; ++x) {
		for(int y = 0; y < _size; ++y) {
			if(get(x,y))
				_grid.set(_x + x,_y + y,filled ? _color : Grid::EMPTY);
		}
	}
}
