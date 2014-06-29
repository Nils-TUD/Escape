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

#include <esc/common.h>
#include <vbe/vbe.h>

class Grid {
public:
	static const int EMPTY	= BLACK;

	explicit Grid(int c,int r) : _cols(c), _rows(r), _grid(new char[_rows * _cols]()) {
	}

	int cols() const {
		return _cols;
	}
	int rows() const {
		return _rows;
	}

	void reset();
	int removeFull();

	char get(int x,int y) const {
		return _grid[y * _cols + x];
	}
	void set(int x,int y,char col) {
		_grid[y * _cols + x] = col;
	}

private:
	bool isFull(int y) const;

	int _cols;
	int _rows;
	char *_grid;
};
