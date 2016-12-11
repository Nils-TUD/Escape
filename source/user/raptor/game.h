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
#include <sys/esccodes.h>

#include "bar.h"
#include "objlist.h"
#include "ui.h"

class Game {
public:
	static const int SCORE_HIT		= 2;
	static const int SCORE_MISS		= -2;
	static const int SCORE_SELFHIT	= -4;

	static const size_t KEYCODE_COUNT	= 128;
	static const int TICK_SLICE			= 10;

	explicit Game(uint cols,uint rows);

	const Bar &bar() const {
		return _bar;
	}
	const ObjList &objects() const {
		return objlist;
	}

	uint getScore() {
		return score;
	}

	void handleKey(uchar keycode,uchar modifiers,A_UNUSED char c) {
		pressed[keycode] = !(modifiers & STATE_BREAK);
	}

	bool tick(time_t time);

private:
	int updateInterval() const {
		return TICK_SLICE / (1000 / timerFreq);
	}
	int keypressInterval() const {
		return updateInterval() * 2;
	}
	int fireInterval() const {
		return updateInterval() * 8;
	}

	bool performAction(time_t time,uchar keycode);
	void fire();
	void addAirplain();

	uint score;
	long timerFreq;
	uint addPlainInt;
	Bar _bar;
	UI ui;
	ObjList objlist;
	uchar pressed[KEYCODE_COUNT];
};
