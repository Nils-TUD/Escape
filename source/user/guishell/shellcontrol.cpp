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
#include <gui/window.h>
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
	if(e.isShiftDown()) {
		switch(e.getKeyCode()) {
			case VK_HOME:
				static_cast<ScrollPane*>(getParent())->scrollToTop();
				return;
			case VK_END:
				static_cast<ScrollPane*>(getParent())->scrollToBottom();
				return;
			case VK_UP:
				static_cast<ScrollPane*>(getParent())->scrollBy(0,
						-(getGraphics()->getFont().getSize().height + PADDING));
				return;
			case VK_DOWN:
				static_cast<ScrollPane*>(getParent())->scrollBy(0,
						getGraphics()->getFont().getSize().height + PADDING);
				return;
			case VK_PGUP:
				static_cast<ScrollPane*>(getParent())->scrollPages(0,-1);
				return;
			case VK_PGDOWN:
				static_cast<ScrollPane*>(getParent())->scrollPages(0,1);
				return;
		}
	}

	uchar modifier = 0;
	if(e.isAltDown())
		modifier |= STATE_ALT;
	if(e.isShiftDown())
		modifier |= STATE_SHIFT;
	if(e.isCtrlDown())
		modifier |= STATE_CTRL;
	vtin_handleKey(_vt,e.getKeyCode(),modifier,e.getCharacter());
	doUpdate();
}

void ShellControl::resizeTo(const Size &size) {
	Control::resizeTo(size);
	size_t parentheight = getParent()->getSize().height - gui::ScrollPane::BAR_SIZE;
	vtctrl_resize(_vt,getCols(size.width),getRows(parentheight));
}

void ShellControl::sendEOF() {
	vtin_handleKey(_vt,VK_C,STATE_CTRL,'c');
	vtin_handleKey(_vt,VK_D,STATE_CTRL,'d');
}

Size ShellControl::getPrefSize() const {
	size_t rows = (HISTORY_SIZE * _vt->rows) - _vt->firstLine;
	Size font = getGraphics()->getFont().getSize();
	return Size(TEXTSTARTX + _vt->cols * font.width,TEXTSTARTY + rows * (font.height + PADDING));
}

Size ShellControl::getUsedSize(const Size &avail) const {
	// we have to take the change in vtctrl_resize into account that will follow based on our result
	size_t rows = (HISTORY_SIZE * _vt->rows) - (_vt->firstLine + (_vt->rows - getRows(avail.height)));
	Size font = getGraphics()->getFont().getSize();
	return Size(avail.width,TEXTSTARTY + rows * (font.height + PADDING));
}

Size ShellControl::rectToLines(const Rectangle &r) const {
	Size font = getGraphics()->getFont().getSize();
	size_t start = r.getPos().y / (font.height + PADDING);
	size_t end = (r.getPos().y + r.getSize().height + font.height + PADDING - 1) / (font.height + PADDING);
	return Size(start, end - start);
}

Rectangle ShellControl::linesToRect(size_t start,size_t count) const {
	Size font = getGraphics()->getFont().getSize();
	Pos pos(0,start == 0 ? 0 : TEXTSTARTY + start * (font.height + PADDING));
	Size size(getSize().width,count * (font.height + PADDING));
	return Rectangle(pos,size);
}

void ShellControl::paint(Graphics &g) {
	locku(&_vt->lock);
	Rectangle paintrect = g.getPaintRect();
	if(!paintrect.empty()) {
		Size dirty = rectToLines(paintrect);

		// fill bg
		g.setColor(BGCOLOR);
		g.fillRect(paintrect.getPos(),paintrect.getSize());

		paintRows(g,dirty.width,dirty.height);
	}
	unlocku(&_vt->lock);
}

void ShellControl::doUpdate() {
	if(_vt->upLength > 0) {
		makeDirty(true);
		getWindow()->layout();
		ScrollPane *sp = static_cast<ScrollPane*>(getParent());
		sp->scrollToBottom();

		if(_vt->upScroll != 0)
			repaint();
		else {
			size_t startRow = _vt->upStart / (_vt->cols * 2);
			size_t rowCount = (_vt->upLength + _vt->cols * 2 - 1) / (_vt->cols * 2);
			Rectangle up = linesToRect(startRow,rowCount);
			Size font = getGraphics()->getFont().getSize();
			repaintRect(up.getPos() - getPos(),up.getSize() + Size(0,font.height + PADDING - 1));
		}
		_vt->upScroll = 0;
		_vt->upStart = 0;
		_vt->upLength = 0;
	}
}

void ShellControl::update() {
	Application::getInstance()->executeLater(make_memfun(this,&ShellControl::doUpdate));
}

bool ShellControl::setCursor() {
	if(_lastCol != _vt->col || _lastRow != _vt->row) {
		Graphics *g = getGraphics();
		Size csize = g->getFont().getSize();
		uint8_t *buf = (uint8_t*)_vt->buffer + ((_vt->firstVisLine + _lastRow) *
				_vt->cols + _lastCol) * 2;
		assert((size_t)(buf[1] >> 4) < ARRAY_SIZE(COLORS));
		// clear old cursor
		g->setColor(COLORS[buf[1] >> 4]);
		g->fillRect(TEXTSTARTX + _lastCol * csize.width,
				TEXTSTARTY + (_lastRow + 1) * (csize.height + PADDING),
				csize.width,CURSOR_WIDTH);

		// draw new one
		g->setColor(CURSOR_COLOR);
		g->fillRect(TEXTSTARTX + _vt->col * csize.width,
				TEXTSTARTY + (_vt->row + 1) * (csize.height + PADDING),
				csize.width,CURSOR_WIDTH);
		_lastCol = _vt->col;
		_lastRow = _vt->row;
		return true;
	}
	return false;
}

void ShellControl::paintRows(Graphics &g,size_t start,size_t count) {
	Size csize = g.getFont().getSize();
	gpos_t y = TEXTSTARTY + start * (csize.height + PADDING);
	char *buf = _vt->buffer + (_vt->firstLine + start) * _vt->cols * 2;

	// paint title-bar?
	if(start == 0) {
		paintRow(g,csize.width,csize.height,_vt->titleBar,y);
		buf += _vt->cols * 2;
		y += csize.height + PADDING;
		count--;
	}

	while(count-- > 0) {
		paintRow(g,csize.width,csize.height,buf,y);
		buf += _vt->cols * 2;
		y += csize.height + PADDING;
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
