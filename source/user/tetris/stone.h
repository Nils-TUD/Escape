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

#include <sys/common.h>
#include <vbe/vbe.h>

#include "grid.h"

class Stone {
public:
	explicit Stone(Grid &grid,int s,char c)
		: _grid(grid), _color(c), _x(grid.cols() / 2 - s / 2), _y(0), _size(s),
		  _fields(new bool[s * s]()) {
	}
	~Stone() {
		delete[] _fields;
	}

	char color() const {
		return _color;
	}
	int size() const {
		return _size;
	}
	bool get(int x,int y) const {
		return _fields[y * _size + x];
	}

	bool left() {
		return move(-1,0);
	}
	bool right() {
		return move(1,0);
	}
	bool down() {
		return move(0,1);
	}
	bool drop() {
		int count = 0;
		while(move(0,1))
			count++;
		return count > 0;
	}
	bool place() {
		if(free()) {
			update(true);
			return true;
		}
		return false;
	}

	bool rotate();

private:
	bool move(int addx,int addy);
	int mostRight() const;
	void update(bool filled);
	bool free() const;

protected:
	void set(int x,int y,bool v) {
		_fields[y * _size + x] = v;
	}

	Grid &_grid;
	char _color;
	int _x;
	int _y;
	int _size;
	bool *_fields;
};

/**
 * +--+--+--+--+
 * |  |  |  |  |
 * +--+--+--+--+
 */
class IStone : public Stone {
public:
	explicit IStone(Grid &grid) : Stone(grid,4,CYAN) {
		for(int i = 0; i < 4; ++i)
			set(i,0,true);
	}
};

/**
 * +--+--+
 * |  |  |
 * +--+--+
 * |  |  |
 * +--+--+
 */
class OStone : public Stone {
public:
	explicit OStone(Grid &grid) : Stone(grid,2,LIGHTORANGE) {
		for(int i = 0; i < 2; ++i) {
			for(int j = 0; j < 2; ++j)
				set(i,j,true);
		}
	}
};

/**
 * +--+
 * |  |
 * +--+--+--+
 * |  |  |  |
 * +--+--+--+
 */
class JStone : public Stone {
public:
	explicit JStone(Grid &grid) : Stone(grid,3,BLUE) {
		set(0,0,true);
		set(0,1,true);
		set(1,1,true);
		set(2,1,true);
	}
};

/**
 *       +--+
 *       |  |
 * +--+--+--+
 * |  |  |  |
 * +--+--+--+
 */
class LStone : public Stone {
public:
	explicit LStone(Grid &grid) : Stone(grid,3,ORANGE) {
		set(0,1,true);
		set(1,1,true);
		set(2,1,true);
		set(2,0,true);
	}
};

/**
 *    +--+--+
 *    |  |  |
 * +--+--+--+
 * |  |  |
 * +--+--+
 */
class SStone : public Stone {
public:
	explicit SStone(Grid &grid) : Stone(grid,3,GREEN) {
		set(0,1,true);
		set(1,0,true);
		set(1,1,true);
		set(2,0,true);
	}
};

/**
 *    +--+
 *    |  |
 * +--+--+--+
 * |  |  |  |
 * +--+--+--+
 */
class TStone : public Stone {
public:
	explicit TStone(Grid &grid) : Stone(grid,3,MARGENTA) {
		set(0,1,true);
		set(1,0,true);
		set(1,1,true);
		set(2,1,true);
	}
};

/**
 * +--+--+
 * |  |  |
 * +--+--+--+
 *    |  |  |
 *    +--+--+
 */
class ZStone : public Stone {
public:
	explicit ZStone(Grid &grid) : Stone(grid,3,RED) {
		set(0,0,true);
		set(1,0,true);
		set(1,1,true);
		set(2,1,true);
	}
};
