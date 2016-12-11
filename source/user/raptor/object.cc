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
#include <assert.h>
#include <stdlib.h>

#include "object.h"
#include "ui.h"

bool Object::collide(Object *o) {
	if((type == BULLET && o->type == AIRPLANE) ||
		(type == AIRPLANE && o->type == BULLET)) {
		return OVERLAPS(x,x + width - 1,o->x,o->x + o->width - 1) &&
			OVERLAPS(y,y + height - 1,o->y,o->y + o->height - 1);
	}
	return false;
}

bool Object::explode() {
	if(type == AIRPLANE) {
		moveCnt = 0;
		type = EXPLO1;
		return true;
	}
	return false;
}

bool Object::tick(UI &ui) {
	moveCnt++;
	if(type == EXPLO1) {
		if(moveCnt == EXPLO_DURATION) {
			moveCnt = 0;
			type = EXPLO2;
		}
		return true;
	}
	if(type == EXPLO2) {
		if(moveCnt == EXPLO_DURATION) {
			moveCnt = 0;
			type = EXPLO3;
		}
		return true;
	}
	if(type == EXPLO3) {
		if(moveCnt == EXPLO_DURATION) {
			moveCnt = 0;
			type = EXPLO4;
		}
		return true;
	}
	if(type == EXPLO4) {
		if(moveCnt == EXPLO_DURATION)
			return false;
		return true;
	}

	if(++moveCnt == speed) {
		moveCnt = 0;
		if(direction & UP) {
			if(y == 0)
				return false;
			y--;
		}
		if(direction & DOWN) {
			if(y + height == (int)ui.gameHeight())
				return false;
			y++;
		}
		if(direction & LEFT) {
			if(x == 0)
				return false;
			x--;
		}
		if(direction & RIGHT) {
			if(x + width == (int)ui.gameWidth())
				return false;
			x++;
		}
	}
	return true;
}
