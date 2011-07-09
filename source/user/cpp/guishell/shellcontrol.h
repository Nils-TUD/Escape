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

#ifndef SHELLCONTROL_H_
#define SHELLCONTROL_H_

#include <esc/common.h>
#include <gui/control.h>
#include <esc/debug.h>
#include <esc/ringbuffer.h>
#include <esc/esccodes.h>
#include <stdlib.h>
#include <iostream>

#include <vterm/vtctrl.h>

using namespace gui;

class ShellApplication;

class ShellControl : public Control {
	friend class ShellApplication;

private:
	static Color COLORS[16];

	static const size_t PADDING = 3;
	static const size_t TEXTSTARTX = 3;
	static const size_t TEXTSTARTY = 3;
	static const size_t CURSOR_WIDTH = 2;
	static const Color BGCOLOR;
	static const Color FGCOLOR;
	static const Color CURSOR_COLOR;

public:
	ShellControl(int sid,gpos_t x,gpos_t y,gsize_t width,gsize_t height) :
		Control(x,y,width,height), _lastCol(0), _lastRow(0), _vt(NULL) {
		int speakerFd;
		sVTSize size;
		Font font;
		size.width = (width - TEXTSTARTX * 2) / font.getWidth();
		size.height = (height - TEXTSTARTY * 2) / (font.getHeight() + PADDING);

		// open speaker
		speakerFd = open("/dev/speaker",IO_MSGS);
		if(speakerFd < 0)
			error("Unable to open '/dev/speaker'");

		_vt = (sVTerm*)malloc(sizeof(sVTerm));
		if(!_vt)
			error("Not enough mem for vterm");
		_vt->index = 0;
		_vt->sid = sid;
		_vt->defForeground = BLACK;
		_vt->defBackground = WHITE;
		if(getenvto(_vt->name,sizeof(_vt->name),"TERM") < 0)
			error("Unable to get env-var TERM");
		if(!vterm_init(_vt,&size,-1,speakerFd))
			error("Unable to init vterm");
		_vt->active = true;
	};
	virtual ~ShellControl() {
		vterm_destroy(_vt);
		free(_vt);
	};

	// no cloning
	ShellControl(const ShellControl &e);
	ShellControl &operator=(const ShellControl &e);

	virtual void paint(Graphics &g);
	inline bool getReadLine() const {
		return _vt->readLine;
	};
	inline bool getEcho() const {
		return _vt->echo;
	};
	inline bool getNavigation() const {
		return _vt->navigation;
	};

private:
	void clearRows(Graphics &g,size_t start,size_t count);
	void paintRows(Graphics &g,size_t start,size_t count);
	void paintRow(Graphics &g,size_t cwidth,size_t cheight,char *buf,gpos_t y);
	void update();
	bool setCursor();
	inline size_t getLineCount() const {
		return (getHeight() / (getGraphics()->getFont().getHeight() + PADDING));
	};
	inline sRingBuf *getInBuf() {
		return _vt->inbuf;
	};
	inline sVTerm *getVTerm() {
		return _vt;
	};

	ushort _lastCol;
	ushort _lastRow;
	sVTerm *_vt;
};

#endif /* SHELLCONTROL_H_ */
