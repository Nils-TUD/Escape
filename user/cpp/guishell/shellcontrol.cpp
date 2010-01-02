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
const Color ShellControl::FGCOLOR = Color(0x0,0x0,0x0);
const Color ShellControl::BORDER_COLOR = Color(0x0,0x0,0x0);
const Color ShellControl::CURSOR_COLOR = Color(0x0,0x0,0x0);

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

void ShellControl::append(char *s,u32 len) {
	char *start = s;
	u32 oldRow = _row;
	_scrollRows = 0;

	s[len] = '\0';

	/* are we waiting to finish an escape-code? */
	if(_escapePos >= 0) {
		u32 oldLen = _escapePos;
		char *escPtr = _escapeBuf;
		u16 length = MIN((s32)len,MAX_ESCC_LENGTH - _escapePos - 1);
		/* append the string */
		memcpy(_escapeBuf + _escapePos,s,length);
		_escapePos += length;
		_escapeBuf[_escapePos] = '\0';

		/* try it again */
		if(!handleEscape(&escPtr)) {
			/* if no space is left, quit and simply print the code */
			if(_escapePos >= MAX_ESCC_LENGTH - 1) {
				u32 i;
				for(i = 0; i < MAX_ESCC_LENGTH; i++)
					append(_escapeBuf[i]);
			}
			/* otherwise try again next time */
			else
				return;
		}
		/* skip escape-code */
		s += (escPtr - _escapeBuf) - oldLen;
		_escapePos = -1;
	}

	while(*s) {
		if(*s == '\033') {
			s++;
			// if the escape-code is incomplete, store what we have so far and wait for further input
			if(!handleEscape((char**)&s)) {
				u32 count = MIN(MAX_ESCC_LENGTH,len - (s - start));
				memcpy(_escapeBuf,s,count);
				_escapePos = count;
				break;
			}
			continue;
		}
		append(*s++);
	}

	// scroll down if necessary
	if(_row > _firstRow + getLineCount()) {
		scrollLine(-_rows.size());
		return;
	}
	if(_row < oldRow) {
		repaint();
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
	requestUpdate();
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

bool ShellControl::handleEscape(char **s) {
	s32 cmd,n1,n2,n3;
	cmd = escc_get((const char**)s,&n1,&n2,&n3);
	if(cmd == ESCC_INCOMPLETE)
		return false;

	switch(cmd) {
		case ESCC_MOVE_LEFT:
			_cursorCol = MAX(0,_cursorCol - n1);
			break;
		case ESCC_MOVE_RIGHT:
			_cursorCol = MIN(_cursorCol - 1,_cursorCol + n1);
			break;
		case ESCC_MOVE_UP:
			_row = MAX(_firstRow,_row - n1);
			break;
		case ESCC_MOVE_DOWN:
			_row = MIN(getLineCount() - 1,_row + n1);
			break;
		case ESCC_MOVE_HOME:
			_cursorCol = 0;
			_row = _firstRow;
			break;
		case ESCC_MOVE_LINESTART:
			_cursorCol = 0;
			break;
		/*case ESCC_DEL_FRONT:
			vterm_delete(vt,n1);
			break;
		case ESCC_DEL_BACK:
			if(vt->readLine) {
				vt->rlBufPos = MIN(vt->rlBufSize - 1,vt->rlBufPos + n1);
				vt->rlBufPos = MIN((u32)vt->cols - vt->rlStartCol,vt->rlBufPos);
				vt->col = vt->rlBufPos + vt->rlStartCol;
			}
			else
				vt->col = MIN(vt->cols - 1,vt->col + n1);
			break;*/
		case ESCC_COLOR:
			if(n1 != ESCC_ARG_UNUSED)
				_color = (_color & 0xF0) | MIN(15,n1);
			else
				_color = (_color & 0xF0) | BLACK;
			if(n2 != ESCC_ARG_UNUSED)
				_color = (_color & 0x0F) | (MIN(15,n2) << 4);
			else
				_color = (_color & 0x0F) | (WHITE << 4);
			break;


		/*case VK_END:
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
		case VK_ESC_NAVI:
			_navigation = value ? true : false;
			break;
		case VK_ESC_BACKUP: {
			if(!_screenBackup)
				_screenBackup = new Vector<Vector<char>*>();
			u32 start = MAX(0,(s32)_rows.size() - getLineCount());
			for(u32 i = start; i < _rows.size(); i++)
				_screenBackup->add(new Vector<char>(*_rows[i]));
		}
		break;
		case VK_ESC_RESTORE:
			if(_screenBackup) {
				u32 start = MAX(0,(s32)_rows.size() - getLineCount());
				for(u32 i = 0; i < _screenBackup->size(); i++) {
					delete _rows[start + i];
					_rows[start + i] = new Vector<char>(*((*_screenBackup)[i]));
				}
				delete _screenBackup;
				_screenBackup = NULL;
				repaint();
			}
			break;*/
	}

	return true;
}
