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

#define TYPE_AIRPLANE	0
#define TYPE_BULLET		1
#define TYPE_EXPLO1		2
#define TYPE_EXPLO2		3
#define TYPE_EXPLO3		4
#define TYPE_EXPLO4		5

#define AIRPLANE_WIDTH	3
#define AIRPLANE_HEIGHT	3

#define BULLET_WIDTH	1
#define BULLET_HEIGHT	1

#define EXPLO_DURATION	8

#define DIR_UP			1
#define DIR_LEFT		2
#define DIR_RIGHT		4
#define DIR_DOWN		8

typedef struct {
	int type;
	int x;
	int y;
	int width;
	int height;
	int direction;
	int speed;
	int moveCnt;
} sObject;

sObject *obj_createAirplain(int x,int y,int direction,int speed);

sObject *obj_createBullet(int x,int y,int direction,int speed);

sObject *obj_create(int type,int x,int y,int width,int height,int direction,int speed);

bool obj_explode(sObject *o);

bool obj_collide(sObject *o1,sObject *o2);

bool obj_tick(sObject *o);

void obj_destroy(sObject *o);
