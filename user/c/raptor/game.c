/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/fileio.h>
#include <esc/conf.h>
#include <esc/keycodes.h>
#include <esc/date.h>
#include <stdlib.h>
#include "game.h"
#include "bar.h"
#include "objlist.h"
#include "display.h"

#define KEYCODE_COUNT		128
#define TICK_SLICE			10
#define UPDATE_INTERVAL		(TICK_SLICE / (1000 / (timerFreq)))
#define KEYPRESS_INTERVAL	(UPDATE_INTERVAL * 2)
#define FIRE_INTERVAL		(UPDATE_INTERVAL * 8)
#define ADDPLAIN_INTERVAL	(UPDATE_INTERVAL * 90)

static bool game_performAction(u32 time,u8 keycode);
static void game_fire(void);
static void game_addAirplain(void);

static u32 score;
static s32 timerFreq;
static u8 pressed[KEYCODE_COUNT];

bool game_init(void) {
	timerFreq = getConf(CONF_TIMER_FREQ);
	if(timerFreq < 0) {
		fprintf(stderr,"Unable to get timer-frequency");
		return false;
	}

	score = 0;
	if(!displ_init())
		return false;
	srand(getTime());
	bar_init();
	objlist_create();
	return true;
}

void game_deinit(void) {
	displ_destroy();
}

u32 game_getScore(void) {
	return score;
}

void game_handleKey(u8 keycode,u8 modifiers,u8 isBreak,char c) {
	UNUSED(modifiers);
	UNUSED(c);
	pressed[keycode] = !isBreak;
}

bool game_tick(u32 time) {
	bool stop = false;
	if((time % UPDATE_INTERVAL) == 0) {
		s32 scoreChg;
		if((time % KEYPRESS_INTERVAL) == 0) {
			u32 i;
			for(i = 0; i < KEYCODE_COUNT; i++) {
				if(pressed[i])
					stop |= !game_performAction(time,i);
			}
		}
		if((time % ADDPLAIN_INTERVAL) == 0)
			game_addAirplain();
		scoreChg = objlist_tick();
		if((s32)(scoreChg + score) < 0)
			score = 0;
		else
			score += scoreChg;
		displ_update();
	}
	return !stop;
}

static bool game_performAction(u32 time,u8 keycode) {
	switch(keycode) {
		case VK_LEFT:
			bar_moveLeft();
			break;
		case VK_RIGHT:
			bar_moveRight();
			break;
		case VK_SPACE:
			if((time % FIRE_INTERVAL) == 0)
				game_fire();
			break;
		case VK_Q:
			return false;
	}
	return true;
}

static void game_fire(void) {
	u32 start,end;
	sObject *o;
	bar_getDim(&start,&end);
	o = obj_createBullet(start + (end - start) / 2,GHEIGHT - 2,DIR_UP,4);
	objlist_add(o);
}

static void game_addAirplain(void) {
	sObject *o;
	u8 x = rand() % (GWIDTH - 2);
	u8 dir = DIR_DOWN;
	switch(rand() % 3) {
		case 0:
			dir |= DIR_LEFT;
			break;
		case 1:
			dir |= DIR_RIGHT;
			break;
	}
	o = obj_createAirplain(x,0,dir,50);
	objlist_add(o);
}
