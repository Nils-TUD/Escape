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

#include <esc/proto/vterm.h>
#include <sys/common.h>
#include <sys/io.h>
#include <sys/messages.h>

class Bar;
class Game;
class ObjList;

class UI {
	static const uint SCORE_WIDTH	= 10;
	static const uint SCORE_HEIGHT	= 4;

public:
	static const uint PADDING 		= 1;

	explicit UI(Game &game,uint cols,uint rows);
	~UI();

	uint width() const {
		return mode.cols;
	}
	uint height() const {
		return mode.rows;
	}
	uint gameWidth() const {
		return width() - PADDING * 2;
	}
	uint gameHeight() const {
		return height() - PADDING * 2;
	}

	void update();

private:
	size_t xyChar(uint x,uint y) const {
		return y * width() * 2 + x * 2;
	}
	size_t xyCol(uint x,uint y) const {
		return y * width() * 2 + x * 2 + 1;
	}

	void drawScore();
	void drawObjects(const ObjList &objlist);
	void drawBar(const Bar &bar);
	void restoreBackup();
	void setBackup();

	static int inputThread(void *arg);

	Game &game;
	esc::UI *ui;
	esc::Screen::Mode mode;
	esc::UIEvents *uiev;
	esc::FrameBuffer *fb;
	uchar *backup;
};
