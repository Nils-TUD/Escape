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

class ObjList;
class UI;

class Object {
	friend class ObjList;

	static const int EXPLO_DURATION		=	8;

public:
	static const int AIRPLANE_WIDTH		=	3;
	static const int AIRPLANE_HEIGHT	=	3;

	static const int BULLET_WIDTH		=	1;
	static const int BULLET_HEIGHT		=	1;

	enum Dir {
		UP		= 0x1,
		LEFT	= 0x2,
		RIGHT	= 0x4,
		DOWN	= 0x8,
	};

	enum Type {
		AIRPLANE,
		BULLET,
		EXPLO1,
		EXPLO2,
		EXPLO3,
		EXPLO4
	};

	static Object *createAirplain(int x,int y,int direction,int speed) {
		return new Object(AIRPLANE,x,y,AIRPLANE_WIDTH,AIRPLANE_HEIGHT,direction,speed);
	}
	static Object *createBullet(int x,int y,int direction,int speed) {
		return new Object(BULLET,x,y,BULLET_WIDTH,BULLET_HEIGHT,direction,speed);
	}

	explicit Object(Type type,int x,int y,int width,int height,int direction,int speed)
		: type(type), x(x), y(y), width(width), height(height), direction(direction), speed(speed),
		  moveCnt(0), _next() {
	}

	Object *next() {
		return _next;
	}

	bool explode();
	bool collide(Object *o);
	bool tick(UI &ui);

	Type type;
	int x;
	int y;
	int width;
	int height;
	int direction;
	int speed;
	int moveCnt;

private:
	Object *_next;
};
