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
#include <stdlib.h>
#include <assert.h>
#include "object.h"
#include "ui.h"

sObject *obj_createAirplain(int x,int y,int direction,int speed) {
	return obj_create(TYPE_AIRPLANE,x,y,AIRPLANE_WIDTH,AIRPLANE_HEIGHT,direction,speed);
}

sObject *obj_createBullet(int x,int y,int direction,int speed) {
	return obj_create(TYPE_BULLET,x,y,BULLET_WIDTH,BULLET_HEIGHT,direction,speed);
}

sObject *obj_create(int type,int x,int y,int width,int height,int direction,int speed) {
	sObject *o = (sObject*)malloc(sizeof(sObject));
	assert(o != NULL);
	o->type = type;
	o->x = x;
	o->y = y;
	o->width = width;
	o->height = height;
	o->direction = direction;
	o->speed = speed;
	o->moveCnt = 0;
	return o;
}

bool obj_collide(sObject *o1,sObject *o2) {
	if((o1->type == TYPE_BULLET && o2->type == TYPE_AIRPLANE) ||
		(o1->type == TYPE_AIRPLANE && o2->type == TYPE_BULLET)) {
		return OVERLAPS(o1->x,o1->x + o1->width - 1,o2->x,o2->x + o2->width - 1) &&
			OVERLAPS(o1->y,o1->y + o1->height - 1,o2->y,o2->y + o2->height - 1);
	}
	return false;
}

bool obj_explode(sObject *o) {
	if(o->type == TYPE_AIRPLANE) {
		o->moveCnt = 0;
		o->type = TYPE_EXPLO1;
		return true;
	}
	return false;
}

bool obj_tick(sObject *o) {
	o->moveCnt++;
	if(o->type == TYPE_EXPLO1) {
		if(o->moveCnt == EXPLO_DURATION) {
			o->moveCnt = 0;
			o->type = TYPE_EXPLO2;
		}
		return true;
	}
	if(o->type == TYPE_EXPLO2) {
		if(o->moveCnt == EXPLO_DURATION) {
			o->moveCnt = 0;
			o->type = TYPE_EXPLO3;
		}
		return true;
	}
	if(o->type == TYPE_EXPLO3) {
		if(o->moveCnt == EXPLO_DURATION) {
			o->moveCnt = 0;
			o->type = TYPE_EXPLO4;
		}
		return true;
	}
	if(o->type == TYPE_EXPLO4) {
		if(o->moveCnt == EXPLO_DURATION)
			return false;
		return true;
	}

	if(++o->moveCnt == o->speed) {
		o->moveCnt = 0;
		if(o->direction & DIR_UP) {
			if(o->y == 0)
				return false;
			o->y--;
		}
		if(o->direction & DIR_DOWN) {
			if(o->y + o->height == (int)GHEIGHT)
				return false;
			o->y++;
		}
		if(o->direction & DIR_LEFT) {
			if(o->x == 0)
				return false;
			o->x--;
		}
		if(o->direction & DIR_RIGHT) {
			if(o->x + o->width == (int)GWIDTH)
				return false;
			o->x++;
		}
	}
	return true;
}

void obj_destroy(sObject *o) {
	free(o);
}
