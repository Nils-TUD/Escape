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

#include <esc/proto/ui.h>
#include <sys/common.h>

#include "grid.h"
#include "stone.h"

class UI {
	enum {
		INFO_COLS		= 21,
		NEXT_OFF		= 1,
		SEP_OFF			= NEXT_OFF + 8,
		LEVEL_OFF		= SEP_OFF + 2,
		SCORE_OFF		= LEVEL_OFF + 2,
		LINES_OFF		= SCORE_OFF + 2,
		MSGBOX_COLS		= 24,
		MSGBOX_ROWS		= 9,
	};

	enum {
		COLOR			= 0x07
	};

public:
	explicit UI(int cols,int rows,int size);
	~UI();

	void start();
	void stop() {
		_run = false;
	}

	int cols() const {
		return _mode.cols;
	}
	int rows() const {
		return _mode.rows;
	}
	int gridCols() {
		return 10;
	}
	int gridRows() {
		return (_mode.rows - 2) / _rpp;
	}
	void update() {
		_ui.update(0,0,_mode.cols,_mode.rows);
	}

	void paintGrid(Grid &grid);
	void paintBorder();
	void paintInfo(Stone *next,int level,int score,int lines);
	void paintMsgBox(const char **lines,size_t count);

private:
	int minCols() {
		return INFO_COLS + gridCols() * _cpp + 2;
	}
	int minRows() {
		return 20;
	}
	int gridStartx() {
		return (cols() - INFO_COLS - gridCols() * _cpp) / 2;
	}
	int gridStarty() {
		return (rows() - gridRows() * _rpp) / 2;
	}
	void set(int x,int y,char c,char col) {
		char *fb = _fb->addr();
		fb[y * cols() * 2 + x * 2] = c;
		fb[y * cols() * 2 + x * 2 + 1] = col;
	}

	void paintRect(int posx,int posy,int width,int height,char color);
	void paintBox(int px,int py,char color);

	static int run(void*);

	esc::UI _ui;
	esc::UI::Mode _mode;
	esc::UIEvents _uievents;
	esc::FrameBuffer *_fb;
	int _cpp;
	int _rpp;
	bool _run;
};
