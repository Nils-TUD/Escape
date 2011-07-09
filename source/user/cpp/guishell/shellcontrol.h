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
#include <esc/esccodes.h>
#include <esc/thread.h>
#include <stdlib.h>

#include <vterm/vtctrl.h>

using namespace gui;

class GUITerm;

class ShellControl : public Control {
	friend class GUITerm;

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
	ShellControl(tULock *lock,gpos_t x,gpos_t y,gsize_t width,gsize_t height) :
		Control(x,y,width,height), _lock(lock), _lastCol(0), _lastRow(0), _vt(NULL) {
	};
	virtual ~ShellControl() {
	};

	// no cloning
	ShellControl(const ShellControl &e);
	ShellControl &operator=(const ShellControl &e);

	virtual void onKeyPressed(const KeyEvent &e);

	inline gsize_t getCols() const {
		return (getWidth() - TEXTSTARTX * 2) / getGraphics()->getFont().getWidth();
	};
	inline gsize_t getRows() const {
		return (getHeight() - TEXTSTARTY * 2) / (getGraphics()->getFont().getHeight() + PADDING);
	};

	void sendEOF();

	virtual void paint(Graphics &g);

private:
	inline void setVTerm(sVTerm *vt) {
		_vt = vt;
	};

	void clearRows(Graphics &g,size_t start,size_t count);
	void paintRows(Graphics &g,size_t start,size_t count);
	void paintRow(Graphics &g,size_t cwidth,size_t cheight,char *buf,gpos_t y);
	void update();
	bool setCursor();

	tULock *_lock;
	ushort _lastCol;
	ushort _lastRow;
	sVTerm *_vt;
};

#endif /* SHELLCONTROL_H_ */
