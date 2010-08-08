/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/gui/common.h>
#include <esc/gui/control.h>
#include <esc/ports.h>
#include <esc/debug.h>
#include <esc/ringbuffer.h>
#include <esc/esccodes.h>
#include <stdlib.h>
#include <iostream>

#include <vterm/vtctrl.h>

using namespace esc;
using namespace esc::gui;

class ShellApplication;

bool handleShortcut(sVTerm *vt,u32 keycode,u8 modifier,char c);

class ShellControl : public Control {
	friend class ShellApplication;

private:
	static Color COLORS[16];

	static const u32 COLUMNS = 80;
	static const u32 ROWS = 25;
	static const u32 PADDING = 3;
	static const u32 TEXTSTARTX = 2;
	static const u32 TEXTSTARTY = 2;
	static const u32 CURSOR_WIDTH = 2;
	static const Color BGCOLOR;
	static const Color FGCOLOR;
	static const Color BORDER_COLOR;
	static const Color CURSOR_COLOR;

public:
	ShellControl(tDrvId sid,tCoord x,tCoord y,tSize width,tSize height) :
		Control(x,y,width,height), _lastCol(0), _lastRow(0), _vt(NULL) {
		tFD speakerFd;
		sVTSize size;
		Font font;
		size.width = (width - 4) / font.getWidth();
		size.height = (height - 4) / (font.getHeight() + PADDING);

		// open speaker
		speakerFd = open("/dev/speaker",IO_WRITE);
		if(speakerFd < 0)
			error("Unable to open '/dev/speaker'");

		_vt = (sVTerm*)malloc(sizeof(sVTerm));
		if(!_vt)
			error("Not enough mem for vterm");
		_vt->index = 0;
		_vt->sid = sid;
		_vt->defForeground = BLACK;
		_vt->defBackground = WHITE;
		if(!getenvto(_vt->name,sizeof(_vt->name),"TERM"))
			error("Unable to get env-var TERM");
		if(!vterm_init(_vt,&size,-1,speakerFd))
			error("Unable to init vterm");
		_vt->active = true;

		_vt->handlerShortcut = handleShortcut;

		// request ports for qemu and bochs
		if(requestIOPort(0xe9) < 0)
			std::cerr << "Unable to request io-port 0xe9" << std::endl;
		if(requestIOPort(0x3f8) < 0)
			std::cerr << "Unable to request io-port 0x3f8" << std::endl;
		if(requestIOPort(0x3fd) < 0)
			std::cerr << "Unable to request io-port 0x3fd" << std::endl;
	};
	virtual ~ShellControl() {
		vterm_destroy(_vt);
		free(_vt);
		releaseIOPort(0xe9);
		releaseIOPort(0x3f8);
		releaseIOPort(0x3fd);
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
	void clearRows(Graphics &g,u32 start,u32 count);
	void paintRows(Graphics &g,u32 start,u32 count);
	void paintRow(Graphics &g,u32 cwidth,u32 cheight,char *buf,tCoord y);
	void update();
	bool setCursor();
	inline u32 getLineCount() const {
		return (getHeight() / (getGraphics()->getFont().getHeight() + PADDING));
	};
	inline sRingBuf *getInBuf() {
		return _vt->inbuf;
	};
	inline sVTerm *getVTerm() {
		return _vt;
	};

	u16 _lastCol;
	u16 _lastRow;
	sVTerm *_vt;
};

#endif /* SHELLCONTROL_H_ */
