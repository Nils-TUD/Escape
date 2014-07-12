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
#include <ipc/proto/ui.h>
#include <ipc/proto/speaker.h>
#include <mutex>

#include "stone.h"
#include "grid.h"
#include "ui.h"

class Game {
	Game() = delete;

	static const int LEVELS		= 10;

	enum State {
		RUNNING,
		ANIMATION,
		PAUSED,
		GAMEOVER
	};

public:
	static void start(int cols,int rows,int size,bool sound);
	static void stop();
	static void handleKey(const ipc::UIEvents::Event &ev);
	static void tick();

private:
	static void reset();
	static Stone *createStone();
	static void newStone();
	static void update(bool all);

	static int _level;
	static int _score;
	static int _lines;
	static long _ticks;
	static State _state;
	static int _removed;
	static UI *_ui;
	static Grid *_grid;
	static ipc::Speaker *_spk;
	static Stone *_cur;
	static Stone *_next;
	static std::mutex _mutex;
};
