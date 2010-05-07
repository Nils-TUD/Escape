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

#include <esc/common.h>
#include <esc/debug.h>
#include <esc/gui/common.h>
#include <esc/gui/color.h>
#include <esc/driver.h>
#include <signal.h>
#include <errors.h>
#include "shellcontrol.h"

#include <vterm/vtin.h>
#include <vterm/vtctrl.h>

using namespace esc;
using namespace esc::gui;

Color ShellControl::COLORS[16] = {
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
const Color ShellControl::BORDER_COLOR = Color(0x0,0x0,0x0);
const Color ShellControl::CURSOR_COLOR = Color(0x0,0x0,0x0);

bool handleShortcut(sVTerm *vt,u32 keycode,u8 modifier,char c) {
	UNUSED(c);
	if(modifier & STATE_CTRL) {
		switch(keycode) {
			case VK_C:
				/* send interrupt to shell (our parent) */
				if(sendSignalTo(getppid(),SIG_INTRPT,0) < 0)
					printe("Unable to send SIG_INTRPT to %d",getppid());
				break;
			case VK_D:
				if(vt->readLine) {
					vt->inbufEOF = true;
					vterm_rlFlushBuf(vt);
				}
				break;
		}
		/* notify the shell (otherwise it won't get the signal directly) */
		if(keycode == VK_C || keycode == VK_D) {
			if(rb_length(vt->inbuf) == 0)
				setDataReadable(vt->sid,true);
			return false;
		}
	}
	return true;
}

void ShellControl::paint(Graphics &g) {
	// fill bg
	g.setColor(BGCOLOR);
	g.fillRect(1,1,getWidth() - 2,getHeight() - 2);
	// draw border
	g.setColor(BORDER_COLOR);
	g.drawRect(0,0,getWidth(),getHeight());

	paintRows(g,0,_vt->rows);
}

void ShellControl::clearRows(Graphics &g,u32 start,u32 count) {
	u32 cheight = g.getFont().getHeight();
	tCoord y = TEXTSTARTY + start * (cheight + PADDING);
	// overwrite with background
	g.setColor(BGCOLOR);
	count = MIN(getLineCount() - start,count);
	g.fillRect(TEXTSTARTX,y,getWidth() - TEXTSTARTX * 2,count * (cheight + PADDING) + 2);
}

void ShellControl::update() {
	bool changed = false;
	Graphics *g = getGraphics();

	if(_vt->upScroll > 0) {
		// move lines up
		if(_vt->upScroll < _vt->rows) {
			u32 lineHeight = g->getFont().getHeight() + PADDING;
			u32 scrollPixel = _vt->upScroll * lineHeight;
			g->moveLines(TEXTSTARTY + scrollPixel + lineHeight,
					getHeight() - scrollPixel - lineHeight - TEXTSTARTY * 2,scrollPixel);
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
		changed = true;
	}
	else if(_vt->upScroll < 0) {
		// move lines down
		if(-_vt->upScroll < _vt->rows) {
			u32 lineHeight = g->getFont().getHeight() + PADDING;
			u32 scrollPixel = -_vt->upScroll * lineHeight;
			g->moveLines(TEXTSTARTY + lineHeight,
					getHeight() - scrollPixel - lineHeight - TEXTSTARTY * 2,-scrollPixel);
		}
		// repaint first lines (not title-bar)
		clearRows(*g,1,-_vt->upScroll);
		paintRows(*g,1,-_vt->upScroll);
		changed = true;
	}
	else if(_vt->upLength > 0) {
		// repaint all dirty lines
		u32 startRow = _vt->upStart / (_vt->cols * 2);
		u32 rowCount = (_vt->upLength + _vt->cols * 2 - 1) / (_vt->cols * 2);
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
}

bool ShellControl::setCursor() {
	if(_lastCol != _vt->col || _lastRow != _vt->row) {
		Graphics *g = getGraphics();
		u32 cwidth = g->getFont().getWidth();
		u32 cheight = g->getFont().getHeight();
		u8 *buf = (u8*)_vt->buffer + ((_vt->firstVisLine + _lastRow) * _vt->cols + _lastCol) * 2;
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

void ShellControl::paintRows(Graphics &g,u32 start,u32 count) {
	u32 cwidth = g.getFont().getWidth();
	u32 cheight = g.getFont().getHeight();
	tCoord y = TEXTSTARTY + start * (cheight + PADDING);
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

void ShellControl::paintRow(Graphics &g,u32 cwidth,u32 cheight,char *buf,tCoord y) {
	u32 lastCol = 0xFFFF;
	// paint char by char because the color might change
	tCoord x = TEXTSTARTX;
	for(u32 j = 0; j < _vt->cols; j++) {
		char c = *buf++;
		u8 col = *buf++;
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
