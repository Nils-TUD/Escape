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
#include <gui/graphics/color.h>
#include <esc/debug.h>
#include <esc/driver.h>
#include <signal.h>
#include <errno.h>
#include "shellcontrol.h"

#include <vterm/vtin.h>
#include <vterm/vtctrl.h>

using namespace gui;

const Color ShellControl::COLORS[16] = {
	Color(0,0,0),
	Color(0,0,168),
	Color(0,168,0),
	Color(0,168,168),
	Color(168,0,0),
	Color(168,0,168),
	Color(168,87,0),
	Color(168,168,168),
	Color(87,87,87),
	Color(87,87,255),
	Color(87,255,87),
	Color(87,255,255),
	Color(255,87,87),
	Color(255,87,255),
	Color(255,255,87),
	Color(255,255,255)
};

const Color ShellControl::BGCOLOR = Color(0xFF,0xFF,0xFF);
const Color ShellControl::FGCOLOR = Color(0x0,0x0,0x0);
const Color ShellControl::CURSOR_COLOR = Color(0x0,0x0,0x0);

void ShellControl::onKeyPressed(const KeyEvent &e) {
	Control::onKeyPressed(e);
	uchar modifier = 0;
	if(e.isAltDown())
		modifier |= STATE_ALT;
	if(e.isShiftDown())
		modifier |= STATE_SHIFT;
	if(e.isCtrlDown())
		modifier |= STATE_CTRL;
	vtin_handleKey(_vt,e.getKeyCode(),modifier,e.getCharacter());
	update();
}

void ShellControl::resizeTo(gsize_t width,gsize_t height) {
	Control::resizeTo(width,height);
	vtctrl_resize(_vt,getCols(),getRows());
	repaint();
}

void ShellControl::sendEOF() {
	vtin_handleKey(_vt,VK_D,STATE_CTRL,'d');
}

gsize_t ShellControl::getMinWidth() const {
	return DEF_WIDTH;
}
gsize_t ShellControl::getMinHeight() const {
	return DEF_HEIGHT;
}

void ShellControl::paint(Graphics &g) {
	// fill bg
	g.setColor(BGCOLOR);
	g.fillRect(0,0,getWidth(),getHeight());

	locku(&_vt->lock);
	paintRows(g,0,_vt->rows);
	unlocku(&_vt->lock);
}

void ShellControl::clearRows(Graphics &g,size_t start,size_t count) {
	gsize_t cheight = g.getFont().getHeight();
	gpos_t y = TEXTSTARTY + start * (cheight + PADDING);
	// overwrite with background
	g.setColor(BGCOLOR);
	count = MIN(getRows() - start,count);
	g.fillRect(TEXTSTARTX,y,getWidth() - TEXTSTARTX * 2,count * (cheight + PADDING) + 2);
}

void ShellControl::update() {
	bool changed = false;
	Graphics *g = getGraphics();

	locku(&_vt->lock);
	if(_vt->upScroll > 0) {
		size_t lineHeight = g->getFont().getHeight() + PADDING;
		// move lines up
		if(_vt->upScroll < _vt->rows) {
			size_t scrollPixel = _vt->upScroll * lineHeight;
			g->moveRows(TEXTSTARTX,TEXTSTARTY + scrollPixel + lineHeight,
					getWidth(),getHeight() - scrollPixel - lineHeight - TEXTSTARTY * 2,scrollPixel);
		}
		// (re-)paint rows below
		if(_vt->rows >= _vt->upScroll) {
			if(_vt->upLength > 0 && _vt->rows > _vt->upScroll) {
				// if the content has changed, too we have to start the refresh one line before
				clearRows(*g,_vt->rows - _vt->upScroll - 1,_vt->upScroll + 1);
				paintRows(*g,_vt->rows - _vt->upScroll - 1,_vt->upScroll + 1);
			}
			else {
				// no content-change, i.e. just scrolling
				clearRows(*g,_vt->rows - _vt->upScroll,_vt->upScroll + 1);
				paintRows(*g,_vt->rows - _vt->upScroll,_vt->upScroll + 1);
			}
		}
		// repaint all
		else {
			clearRows(*g,0,_vt->rows);
			paintRows(*g,0,_vt->rows);
		}

		// fill the bg of the left few pixels at the bottom that are not affected
		g->setColor(BGCOLOR);
		gpos_t start = TEXTSTARTY + _vt->rows * lineHeight;
		g->fillRect(0,start,getWidth(),getHeight() - start);

		changed = true;
	}
	else if(_vt->upScroll < 0) {
		// move lines down
		if(-_vt->upScroll < _vt->rows) {
			size_t lineHeight = g->getFont().getHeight() + PADDING;
			size_t scrollPixel = -_vt->upScroll * lineHeight;
			g->moveRows(TEXTSTARTX,TEXTSTARTY + lineHeight,
					getWidth(),getHeight() - scrollPixel - lineHeight - TEXTSTARTY * 2,-scrollPixel);
		}
		// repaint first lines (not title-bar)
		clearRows(*g,1,-_vt->upScroll);
		paintRows(*g,1,-_vt->upScroll);
		changed = true;
	}
	else if(_vt->upLength > 0) {
		// repaint all dirty lines
		size_t startRow = _vt->upStart / (_vt->cols * 2);
		size_t rowCount = (_vt->upLength + _vt->cols * 2 - 1) / (_vt->cols * 2);
		clearRows(*g,startRow,rowCount);
		paintRows(*g,startRow,rowCount);
		changed = true;
	}
	changed |= setCursor();
	if(changed)
		requestUpdate();

	// all synchronized now
	_vt->upStart = 0;
	_vt->upLength = 0;
	_vt->upScroll = 0;
	unlocku(&_vt->lock);
}

bool ShellControl::setCursor() {
	if(_lastCol != _vt->col || _lastRow != _vt->row) {
		Graphics *g = getGraphics();
		size_t cwidth = g->getFont().getWidth();
		size_t cheight = g->getFont().getHeight();
		uint8_t *buf = (uint8_t*)_vt->buffer + ((_vt->firstVisLine + _lastRow) *
				_vt->cols + _lastCol) * 2;
		// clear old cursor
		g->setColor(COLORS[buf[1] >> 4]);
		g->fillRect(TEXTSTARTX + _lastCol * cwidth,
				TEXTSTARTY + (_lastRow + 1) * (cheight + PADDING),
				cwidth,CURSOR_WIDTH);

		// draw new one
		g->setColor(CURSOR_COLOR);
		g->fillRect(TEXTSTARTX + _vt->col * cwidth,
				TEXTSTARTY + (_vt->row + 1) * (cheight + PADDING),
				cwidth,CURSOR_WIDTH);
		_lastCol = _vt->col;
		_lastRow = _vt->row;
		return true;
	}
	return false;
}

void ShellControl::paintRows(Graphics &g,size_t start,size_t count) {
	size_t cwidth = g.getFont().getWidth();
	size_t cheight = g.getFont().getHeight();
	gpos_t y = TEXTSTARTY + start * (cheight + PADDING);
	char *buf = _vt->buffer + (_vt->firstVisLine + start) * _vt->cols * 2;
	count = MIN(count,_vt->rows - start);

	// paint title-bar?
	if(start == 0) {
		paintRow(g,cwidth,cheight,_vt->titleBar,y);
		buf += _vt->cols * 2;
		y += cheight + PADDING;
		count--;
	}

	while(count-- > 0) {
		paintRow(g,cwidth,cheight,buf,y);
		buf += _vt->cols * 2;
		y += cheight + PADDING;
	}
}

void ShellControl::paintRow(Graphics &g,size_t cwidth,size_t cheight,char *buf,gpos_t y) {
	size_t lastCol = -1;
	// paint char by char because the color might change
	gpos_t x = TEXTSTARTX;
	for(size_t j = 0; j < _vt->cols; j++) {
		char c = *buf++;
		uchar col = *buf++;
		// color-change?
		if(col != lastCol) {
			lastCol = col;
			g.setColor(COLORS[lastCol & 0xF]);
		}
		// draw background
		if(lastCol >> 4 != WHITE) {
			g.setColor(COLORS[lastCol >> 4]);
			g.fillRect(x,y,cwidth,cheight + PADDING);
			g.setColor(COLORS[lastCol & 0xF]);
		}
		// draw char
		g.drawChar(x,y + PADDING,c);
		x += cwidth;
	}
}
