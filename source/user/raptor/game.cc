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
#include <sys/esccodes.h>
#include <sys/keycodes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bar.h"
#include "game.h"
#include "objlist.h"
#include "ui.h"

Game::Game(uint cols,uint rows)
		: score(0), timerFreq(sysconf(CONF_TIMER_FREQ)), addPlainInt(updateInterval() * 90),
		  _bar(), ui(*this,cols,rows), objlist(), pressed() {
	if(timerFreq < 0)
		error("Unable to get timer-frequency");

	srand(time(NULL));
}

bool Game::tick(time_t gtime) {
	bool stop = false;

	if((gtime % updateInterval()) == 0) {
		int scoreChg;
		if((gtime % keypressInterval()) == 0) {
			size_t i;
			for(i = 0; i < KEYCODE_COUNT; i++) {
				if(pressed[i])
					stop |= !performAction(gtime,i);
			}
		}

		if((gtime % addPlainInt) == 0) {
			if(addPlainInt > 20)
				addPlainInt--;
			addAirplain();
		}

		scoreChg = objlist.tick(ui);
		if((int)(scoreChg + score) < 0)
			score = 0;
		else
			score += scoreChg;

		ui.update();
	}

	return !stop;
}

bool Game::performAction(time_t gtime,uchar keycode) {
	switch(keycode) {
		case VK_LEFT:
			_bar.moveLeft();
			break;
		case VK_RIGHT:
			_bar.moveRight(ui);
			break;
		case VK_SPACE:
			if((gtime % fireInterval()) == 0)
				fire();
			break;
		case VK_Q:
			return false;
	}
	return true;
}

void Game::fire() {
	size_t start,end;
	_bar.getDim(&start,&end);
	objlist.add(Object::createBullet(start + (end - start) / 2,ui.gameHeight() - 2,Object::UP,4));
}

void Game::addAirplain() {
	uint x = rand() % (ui.gameWidth() - 2);
	uint dir = Object::DOWN;
	switch(rand() % 3) {
		case 0:
			dir |= Object::LEFT;
			break;
		case 1:
			dir |= Object::RIGHT;
			break;
	}
	objlist.add(Object::createAirplain(x,0,dir,50));
}
