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
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "progress.h"

using namespace std;

Progress::Progress(size_t startSkip,size_t finished,size_t itemCount)
	: _show(!sysconf(CONF_LOG_TO_VGA)), _startSkip(startSkip), _itemCount(itemCount),
	  _finished(finished), _scr(), _fb() {
}

Progress::~Progress() {
	delete _scr;
	delete _fb;
}

void Progress::itemStarting(const string& s) {
	if(_show && connect()) {
		// copy text to bar
		memclear(_emptyBar,sizeof(_emptyBar));
		for(size_t i = 0; i < s.length(); i++) {
			_emptyBar[i * 2] = s[i];
			_emptyBar[i * 2 + 1] = 0x07;
		}
		// send to screen
		size_type y = getPadY() + BAR_HEIGHT + 2 + BAR_TEXT_PAD;
		paintTo(_emptyBar,getPadX(),y,sizeof(_emptyBar) / 2,1);
	}
}

void Progress::itemLoaded() {
	if(_finished < _itemCount)
		_finished++;
	updateBar();
}

void Progress::itemTerminated() {
	if(_finished > 0)
		_finished--;
	updateBar();
}

void Progress::updateBar() {
	if(_show && connect()) {
		// fill bar
		size_type skipSize = (size_type)((BAR_WIDTH - 3) * (_startSkip / 100.0f));
		size_type fillSize = BAR_WIDTH - 3 - skipSize;
		size_type filled = (size_type)(fillSize * ((float)_finished / _itemCount));
		memclear(_emptyBar,sizeof(_emptyBar));
		for(size_t i = 0; i < fillSize; i++) {
			_emptyBar[i * 2] = ' ';
			_emptyBar[i * 2 + 1] = i < filled ? 0x70 : 0x07;
		}
		// send to screen
		paintTo(_emptyBar,getPadX() + 1 + skipSize,getPadY() + 1,fillSize,1);
	}
}

void Progress::paintBar() {
	if(_show && connect()) {
		/* clear screen */
		char *zeros = (char*)calloc(_fb->mode().cols * _fb->mode().rows * 2,1);
		paintTo(zeros,0,0,_fb->mode().cols,_fb->mode().rows);
		free(zeros);

		const char color = 0x07;
		// top
		_emptyBar[0] = '\xC9';
		_emptyBar[1] = color;
		for(size_t x = 1; x < BAR_WIDTH - 2; x++) {
			_emptyBar[x * 2] = '\xCD';
			_emptyBar[x * 2 + 1] = color;
		}
		_emptyBar[(BAR_WIDTH - 2) * 2] = '\xBB';
		_emptyBar[(BAR_WIDTH - 2) * 2 + 1] = color;
		paintTo(_emptyBar,getPadX(),getPadY(),BAR_WIDTH - 1,1);

		// bottom
		_emptyBar[0] = '\xC8';
		_emptyBar[1] = color;
		_emptyBar[(BAR_WIDTH - 2) * 2] = '\xBC';
		_emptyBar[(BAR_WIDTH - 2) * 2 + 1] = color;
		paintTo(_emptyBar,getPadX(),getPadY() + BAR_HEIGHT + 1,BAR_WIDTH - 1,1);

		// left and right
		memclear(_emptyBar,sizeof(_emptyBar));
		for(size_t y = 0; y < BAR_HEIGHT; y++) {
			_emptyBar[0] = '\xBA';
			_emptyBar[1] = color;
			_emptyBar[(BAR_WIDTH - 2) * 2] = '\xBA';
			_emptyBar[(BAR_WIDTH - 2) * 2 + 1] = color;
			paintTo(_emptyBar,getPadX(),getPadY() + y + 1,BAR_WIDTH - 1,1);
		}

		// initial fill
		for(size_t i = 0; i < _startSkip; i++) {
			_emptyBar[i * 2] = ' ';
			_emptyBar[i * 2 + 1] = 0x70;
		}
		paintTo(_emptyBar,getPadX() + 1,getPadY() + 1,_startSkip,1);
	}
	updateBar();
}

void Progress::paintTo(const void *data,int x,int y,size_t width,size_t height) {
	if(_show) {
		memcpy(_fb->addr() + _fb->mode().cols * y * 2 + x * 2,data,width * height * 2);
		_scr->update(x,y,width,height);
	}
}

bool Progress::connect() {
	if(_scr)
		return true;

	struct stat info;
	if(stat("/dev/vga",&info) < 0)
		return false;

	// if this fails, try again later
	try {
		_scr = new esc::Screen("/dev/vga");
	}
	catch(...) {
		return false;
	}

	// this should succeed. if it does not, it's ok to die
	esc::Screen::Mode mode = _scr->findTextMode(VGA_COLS,VGA_ROWS);
	_fb = new esc::FrameBuffer(mode,"init-vga",esc::Screen::MODE_TYPE_TUI,0644);
	_scr->setMode(esc::Screen::MODE_TYPE_TUI,mode.id,"init-vga",true);
	paintBar();
	return true;
}
