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
#include <esc/messages.h>
#include <string>

class Progress {
private:
	typedef size_t size_type;

private:
	static const uint VGA_COLS			= 80;
	static const uint VGA_ROWS			= 25;

	static const size_type BAR_HEIGHT	= 1;
	static const size_type BAR_WIDTH	= 60;
	static const size_type BAR_PAD		= 10;
	static const size_type BAR_TEXT_PAD	= 1;

public:
	Progress(size_t startSkip,size_t finished,size_t itemCount);
	~Progress();

	void paintBar();
	void itemStarting(const std::string& s);
	void itemLoaded();
	void itemTerminated();
	void allTerminated() {
		_finished = 0;
		updateBar();
	}

private:
	size_type getPadX() const {
		return (_mode.cols - BAR_WIDTH) / 2;
	}
	size_type getPadY() const {
		return (_mode.rows / 2) - ((BAR_HEIGHT + 2) / 2) - 1;
	}
	void updateBar();
	void paintTo(const void *data,int x,int y,size_t width,size_t height);
	bool connect();

private:
	int _fd;
	char *_shm;
	size_t _startSkip;
	size_t _itemCount;
	size_t _finished;
	sScreenMode _mode;
	char _emptyBar[BAR_WIDTH * 2];
};
