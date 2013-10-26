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

#pragma once

#include <esc/common.h>
#include <gui/control.h>
#include <gui/scrollpane.h>
#include <esc/esccodes.h>
#include <esc/thread.h>
#include <stdlib.h>

#include <vterm/vtctrl.h>

class GUITerm;

class ShellControl : public gui::Control {
	friend class GUITerm;

private:
	static const gui::Color COLORS[16];

	static const gsize_t PADDING		= 3;
	static const gsize_t TEXTSTARTX		= 3;
	static const gsize_t TEXTSTARTY		= 3;
	static const gsize_t CURSOR_HEIGHT	= 2;
	static const gui::Color BGCOLOR;
	static const gui::Color FGCOLOR;
	static const gui::Color CURSOR_COLOR;

	static gui::Size getSizeFor(const gui::Size &fontSize,uint cols,uint rows) {
		return gui::Size(TEXTSTARTX * 2 + cols * fontSize.width,
			TEXTSTARTY * 2 + rows * (fontSize.height + PADDING));
	}

public:
	ShellControl() : Control(), _lastCol(0), _lastRow(0), _locked(), _vt(nullptr) {
	}
	virtual ~ShellControl() {
	}

	// no cloning
	ShellControl(const ShellControl &e);
	ShellControl &operator=(const ShellControl &e);

	virtual void onKeyPressed(const gui::KeyEvent &e);
	virtual bool resizeTo(const gui::Size &size);

	gsize_t getCols(size_t avail) const {
		size_t fontwidth = getGraphics()->getFont().getSize().width;
		return (avail - TEXTSTARTX * 2) / fontwidth;
	}
	gsize_t getRows(size_t avail) const {
		size_t fontheight = getGraphics()->getFont().getSize().height;
		return (avail - TEXTSTARTY * 2) / (fontheight + PADDING);
	}

	void sendEOF();
	virtual void paint(gui::Graphics &g);

private:
	virtual gui::Size getPrefSize() const;
	virtual gui::Size getUsedSize(const gui::Size &avail) const;
	gui::Size rectToLines(const gui::Rectangle &r) const;
	gui::Rectangle linesToRect(size_t start,size_t count) const;
	void setVTerm(sVTerm *vt) {
		_vt = vt;
	}

	void paintRows(gui::Graphics &g,size_t start,size_t count);
	void paintRow(gui::Graphics &g,size_t cwidth,size_t cheight,char *buf,gpos_t y);
	void update();
	void doUpdate();
	void setCursor() {
		locku(&_vt->lock);
		doSetCursor();
		unlocku(&_vt->lock);
	}
	void doSetCursor();
	void resizeVTerm(const gui::Size &size);

	size_t _lastCol;
	size_t _lastRow;
	bool _locked;
	sVTerm *_vt;
};
