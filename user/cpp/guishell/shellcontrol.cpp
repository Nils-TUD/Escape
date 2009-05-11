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
#include <esc/gui/common.h>
#include <esc/gui/color.h>
#include "shellcontrol.h"

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
const Color ShellControl::FGCOLOR = Color(0,0,0);
const Color ShellControl::BORDER_COLOR = Color(0,0,0);
const Color ShellControl::CURSOR_COLOR = Color(0,0,0);

void ShellControl::paint(Graphics &g) {
	// fill bg
	g.setColor(BGCOLOR);
	g.fillRect(1,1,getWidth() - 2,getHeight() - 2);
	// draw border
	g.setColor(BORDER_COLOR);
	g.drawRect(0,0,getWidth(),getHeight());

	paintRows(g,0,_rows.size());
}

void ShellControl::clearRows(Graphics &g,u32 start,u32 count) {
	u32 cheight = g.getFont().getHeight();
	tCoord y = start * (cheight + PADDING);
	// overwrite with background
	g.setColor(BGCOLOR);
	count = MIN(getLineCount() - start,count);
	g.fillRect(1,1 + y,getWidth() - 2,count * (cheight + PADDING));
}

void ShellControl::paintRows(Graphics &g,u32 start,u32 count) {
	u32 cwidth = g.getFont().getWidth();
	u32 cheight = g.getFont().getHeight();
	tCoord x;
	tCoord y = 1 + start * (cheight + PADDING);
	Vector<char> &first = *_rows[start + _firstRow];
	u8 lastCol = first.size() > 1 ? first[1] : (WHITE << 4 | BLACK);
	count = MIN(getLineCount() - start,count);

	g.setColor(COLORS[lastCol & 0xF]);
	for(u32 i = start + _firstRow, end = start + _firstRow + count; i < end; i++) {
		// paint char by char because the color might change
		x = PADDING;
		Vector<char> &str = *_rows[i];
		for(u32 j = 0; j < str.size(); j += 2) {
			// color-change?
			if((u8)(str[j + 1]) != lastCol) {
				lastCol = (u8)(str[j + 1]);
				g.setColor(COLORS[lastCol & 0xF]);
			}
			// draw background
			if(lastCol >> 4 != WHITE) {
				g.setColor(COLORS[lastCol >> 4]);
				g.fillRect(x,y,cwidth,cheight + PADDING);
				g.setColor(COLORS[lastCol & 0xF]);
			}
			// draw char
			g.drawChar(x,y + PADDING,str[j]);
			x += cwidth;
		}
		y += cheight + PADDING;
	}

	// paint cursor
	if(_row < _firstRow + getLineCount()) {
		g.fillRect(PADDING + _cursorCol * cwidth,PADDING + (_row - _firstRow) * (cheight + PADDING),
				CURSOR_WIDTH,cheight);
	}
}

void ShellControl::scrollPage(s32 up) {
	scrollLine(up * getLineCount());
}

void ShellControl::scrollLine(s32 up) {
	if(up > 0) {
		if((s32)_firstRow > up)
			_firstRow -= up;
		else
			_firstRow = 0;
	}
	else {
		u32 lines = -up;
		if(_firstRow + lines < _rows.size() - getLineCount())
			_firstRow += lines;
		else
			_firstRow = _rows.size() - getLineCount();
	}
	repaint();
}

void ShellControl::append(const char *s) {
	u32 oldRow = _row;
	_scrollRows = 0;

	while(*s) {
		if(*s == '\033') {
			handleEscape(s + 1);
			s += 3;
			continue;
		}
		append(*s);
		s++;
	}

	// scroll down if necessary
	if(_row > _firstRow + getLineCount()) {
		scrollLine(-_rows.size());
		return;
	}

	Graphics *g = getGraphics();
	if(_scrollRows) {
		// move lines up
		u32 lineHeight = g->getFont().getHeight() + PADDING;
		u32 scrollPixel = _scrollRows * lineHeight;
		g->moveLines(scrollPixel,getHeight() - scrollPixel - 1,scrollPixel);
		// paint new rows
		clearRows(*g,_row - _scrollRows - _firstRow,_scrollRows + 1);
		paintRows(*g,_row - _scrollRows - _firstRow,_scrollRows + 1);
	}
	else {
		// just repaint the changed rows
		clearRows(*g,oldRow - _firstRow,_row - oldRow + 1);
		paintRows(*g,oldRow - _firstRow,_row - oldRow + 1);
	}
	g->update();
}

void ShellControl::append(char c) {
	// write to bochs/qemu console (\r not necessary here)
	if(c != '\r' && c != '\a' && c != '\b' && c != '\t') {
		outByte(0xe9,c);
		outByte(0x3f8,c);
		while((inByte(0x3fd) & 0x20) == 0)
			;
	}

	switch(c) {
		case '\t': {
			u32 i = TAB_WIDTH - _col % TAB_WIDTH;
			while(i-- > 0)
				append(' ');
		}
		break;

		case '\b':
			if(_col > 0) {
				// delete last char
				_rows[_row]->remove((_col - 1) * 2,2);
				_col--;
				_cursorCol--;
			}
			break;

		case '\n':
			_row++;
			if(_row >= _rows.size()) {
				// remove first row, if we're reached the end
				if(_row >= getLineCount()) {
					if(_row >= HISTORY_SIZE) {
						delete _rows[0];
						_rows.remove(0);
						_row--;
					}
					else
						_firstRow++;
					_scrollRows++;
				}
				_rows.add(new Vector<char>(COLUMNS * 2));
			}
			_col = 0;
			_cursorCol = 0;
			break;

		case '\r':
			_col = 0;
			_cursorCol = 0;
			break;

		default:
			// auto newline required?
			if(_cursorCol >= COLUMNS)
				append('\n');

			Vector<char> *str = _rows[_row];
			// replace it, if we're not at the end
			if(_cursorCol < str->size() / 2)
				str->remove(_cursorCol * 2,2);
			else
				_col++;

			// insert char and color
			str->insert(_cursorCol * 2,c);
			str->insert(_cursorCol * 2 + 1,_color);
			_cursorCol++;
			break;
	}
}

bool ShellControl::handleEscape(const char *s) {
	u8 keycode = *(u8*)s;
	u8 value = *(u8*)(s + 1);
	bool res = false;
	switch(keycode) {
		case VK_LEFT:
			if(_cursorCol > 0)
				_cursorCol--;
			res = true;
			break;
		case VK_RIGHT:
			if(_cursorCol < COLUMNS - 1)
				_cursorCol++;
			res = true;
			break;
		case VK_HOME:
			if(value > 0) {
				if(value > _cursorCol)
					_cursorCol = 0;
				else
					_cursorCol -= value;
			}
			res = true;
			break;
		case VK_END:
			if(value > 0) {
				if(_cursorCol + value > COLUMNS - 1)
					_cursorCol = COLUMNS - 1;
				else
					_cursorCol += value;
			}
			res = true;
			break;
		case VK_ESC_RESET:
			_color = (WHITE << 4) | BLACK;
			res = true;
			break;
		case VK_ESC_FG:
			_color = (_color & 0xF0) | MIN(15,value);
			res = true;
			break;
		case VK_ESC_BG:
			_color = (_color & 0x0F) | (MIN(15,value) << 4);
			res = true;
			break;
	}

	return res;
}
