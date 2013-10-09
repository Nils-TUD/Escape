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
#include <esc/driver/video.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/mem.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "progress.h"

using namespace std;

Progress::Progress(size_t startSkip,size_t finished,size_t itemCount)
		: _fd(-1), _shm(), _startSkip(startSkip), _itemCount(itemCount), _finished(finished),
		  _vtSize(sVTSize()) {
}

Progress::~Progress() {
	if(_shm)
		munmap(_shm);
	if(_fd >= 0)
		close(_fd);
}

void Progress::itemStarting(const string& s) {
	if(connect()) {
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
		paintTo(_emptyBar,getPadX() + 1 + skipSize,getPadY() + 1,fillSize,1);
	}
}

void Progress::paintBar() {
	if(connect()) {
		/* clear screen */
		char *zeros = (char*)calloc(_vtSize.width * _vtSize.height * 2,1);
		paintTo(zeros,0,0,_vtSize.width,_vtSize.height);
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
	}
	updateBar();
}

void Progress::paintTo(const void *data,int x,int y,size_t width,size_t height) {
	uint start = _vtSize.width * y * 2 + x * 2;
	memcpy(_shm + start,data,width * height * 2);
	video_update(_fd,start,width * height * 2);
}

bool Progress::connect() {
	if(_fd >= 0)
		return true;
	_fd = open("/dev/vga",IO_WRITE | IO_MSGS);
	if(_fd < 0)
		return false;

	ssize_t modeCnt = video_getModeCount(_fd);
	if(modeCnt < 0)
		error("Unable to get mode count");
	sVTMode *modes = new sVTMode[modeCnt];
	if(video_getModes(_fd,modes,modeCnt) < 0)
		error("Unable to get modes");
	for(ssize_t i = 0; i < modeCnt; ++i) {
		if(modes[i].width >= VGA_COLS && modes[i].height >= VGA_ROWS) {
			int fd = shm_open("init-vga",IO_READ | IO_WRITE | IO_CREATE,0777);
			if(fd < 0)
				error("Unable to open vga shm");
			_shm = (char*)mmap(NULL,modes[i].width * modes[i].height * 2,0,PROT_READ | PROT_WRITE,
				MAP_SHARED,fd,0);
			close(fd);
			if(video_setMode(_fd,modes[i].id,"init-vga",true) < 0)
				error("Unable to set mode");
			_vtSize.width = modes[i].width;
			_vtSize.height = modes[i].height;
			break;
		}
	}
	if(!_shm)
		error("Unable to mmap vga shm");
	delete[] modes;
	return true;
}
