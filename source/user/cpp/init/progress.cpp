/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <esc/messages.h>
#include <esc/io.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "progress.h"

Progress::Progress(size_t startSkip,size_t finished,size_t itemCount)
		: _fd(-1), _startSkip(startSkip), _itemCount(itemCount), _finished(finished),
		  _vtSize(sVTSize()) {
}

Progress::~Progress() {
	if(_fd >= 0)
		close(_fd);
}

void Progress::itemStarting(const std::string& s) {
	if(connect()) {
		// copy text to bar
		memclear(_emptyBar,sizeof(_emptyBar));
		for(size_t i = 0; i < s.length(); i++) {
			_emptyBar[i * 2] = s[i];
			_emptyBar[i * 2 + 1] = 0x07;
		}
		// send to screen
		size_type y = getPadY() + BAR_HEIGHT + 2 + BAR_TEXT_PAD;
		paintTo(_emptyBar,getPadX(),y,sizeof(_emptyBar));
	}
}

void Progress::itemLoaded() {
	_finished++;
	updateBar();
}

void Progress::itemTerminated() {
	_finished--;
	updateBar();
}

void Progress::updateBar() {
	if(connect()) {
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
		paintTo(_emptyBar,getPadX() + 1 + skipSize,getPadY() + 1,fillSize * 2);
	}
}

void Progress::paintBar() {
	if(connect()) {
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
		paintTo(_emptyBar,getPadX(),getPadY(),(BAR_WIDTH - 1) * 2);

		// bottom
		_emptyBar[0] = '\xC8';
		_emptyBar[1] = color;
		_emptyBar[(BAR_WIDTH - 2) * 2] = '\xBC';
		_emptyBar[(BAR_WIDTH - 2) * 2 + 1] = color;
		paintTo(_emptyBar,getPadX(),getPadY() + BAR_HEIGHT + 1,(BAR_WIDTH - 1) * 2);

		// left and right
		memclear(_emptyBar,sizeof(_emptyBar));
		for(size_t y = 0; y < BAR_HEIGHT; y++) {
			_emptyBar[0] = '\xBA';
			_emptyBar[1] = color;
			_emptyBar[(BAR_WIDTH - 2) * 2] = '\xBA';
			_emptyBar[(BAR_WIDTH - 2) * 2 + 1] = color;
			paintTo(_emptyBar,getPadX(),getPadY() + y + 1,(BAR_WIDTH - 1) * 2);
		}
	}
	updateBar();
}

void Progress::paintTo(const void *data,int x,int y,size_t size) {
	if(seek(_fd,_vtSize.width * y * 2 + x * 2,SEEK_SET) < 0)
		cerr << "Unable to seek in video" << endl;
	if(write(_fd,data,size) < 0)
		cerr << "Unable to write to video" << endl;
}

bool Progress::connect() {
	sMsg msg;
	if(_fd >= 0)
		return true;
	_fd = open("/dev/video",IO_WRITE | IO_MSGS);
	if(_fd < 0)
		return false;
	if(send(_fd,MSG_VID_GETSIZE,NULL,0) < 0)
		error("Unable to send getsize-message to video-driver");
	if(RETRY(receive(_fd,NULL,&msg,sizeof(msg))) < 0)
		error("Unable to receive size of screen");
	memcpy(&_vtSize,&msg.data.d,sizeof(sVTSize));
	return true;
}
