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
#include <sys/conf.h>
#include <sys/keycodes.h>
#include <sys/esccodes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "game.h"
#include "bar.h"
#include "objlist.h"
#include "ui.h"

#define KEYCODE_COUNT		128
#define TICK_SLICE			10
#define UPDATE_INTERVAL		(TICK_SLICE / (1000 / (timerFreq)))
#define KEYPRESS_INTERVAL	(UPDATE_INTERVAL * 2)
#define FIRE_INTERVAL		(UPDATE_INTERVAL * 8)
#define ADDPLAIN_INTERVAL	(UPDATE_INTERVAL * 90)

static bool game_performAction(time_t time,uchar keycode);
static void game_fire(void);
static void game_addAirplain(void);

static uint score;
static long timerFreq;
static uchar pressed[KEYCODE_COUNT];
static uint addPlainInt;

bool game_init(uint cols,uint rows) {
	timerFreq = sysconf(CONF_TIMER_FREQ);
	if(timerFreq < 0)
		error("Unable to get timer-frequency");
	addPlainInt = ADDPLAIN_INTERVAL;

	score = 0;
	ui_init(cols,rows);
	srand(time(NULL));
	bar_init();
	objlist_create();
	return true;
}

void game_deinit(void) {
	ui_destroy();
}

uint game_getScore(void) {
	return score;
}

void game_handleKey(uchar keycode,uchar modifiers,A_UNUSED char c) {
	pressed[keycode] = !(modifiers & STATE_BREAK);
}

bool game_tick(time_t gtime) {
	bool stop = false;
	if((gtime % UPDATE_INTERVAL) == 0) {
		int scoreChg;
		if((gtime % KEYPRESS_INTERVAL) == 0) {
			size_t i;
			for(i = 0; i < KEYCODE_COUNT; i++) {
				if(pressed[i])
					stop |= !game_performAction(gtime,i);
			}
		}
		if((gtime % addPlainInt) == 0) {
			if(addPlainInt > 20)
				addPlainInt--;
			game_addAirplain();
		}
		scoreChg = objlist_tick();
		if((int)(scoreChg + score) < 0)
			score = 0;
		else
			score += scoreChg;
		ui_update();
	}
	return !stop;
}

static bool game_performAction(time_t gtime,uchar keycode) {
	switch(keycode) {
		case VK_LEFT:
			bar_moveLeft();
			break;
		case VK_RIGHT:
			bar_moveRight();
			break;
		case VK_SPACE:
			if((gtime % FIRE_INTERVAL) == 0)
				game_fire();
			break;
		case VK_Q:
			return false;
	}
	return true;
}

static void game_fire(void) {
	size_t start,end;
	sObject *o;
	bar_getDim(&start,&end);
	o = obj_createBullet(start + (end - start) / 2,GHEIGHT - 2,DIR_UP,4);
	objlist_add(o);
}

static void game_addAirplain(void) {
	sObject *o;
	uint x = rand() % (GWIDTH - 2);
	uint dir = DIR_DOWN;
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
