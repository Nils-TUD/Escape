/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <sys/common.h>
#include <gui/editable.h>
#include <sys/keycodes.h>

using namespace std;

namespace gui {
	const uchar Editable::DIR_NONE			= 0;
	const uchar Editable::DIR_LEFT			= 1;
	const uchar Editable::DIR_RIGHT			= 2;

	const gsize_t Editable::CURSOR_WIDTH	= 2;
	const gsize_t Editable::CURSOR_OVERLAP	= 2;
	const gsize_t Editable::DEF_WIDTH		= 100;

	Size Editable::getPrefSize() const {
		gsize_t pad = getTheme().getTextPadding();
		return Size(DEF_WIDTH + pad * 2,getGraphics()->getFont().getSize().height + pad * 2);
	}

	void Editable::onFocusGained() {
		Control::onFocusGained();
		setFocused(true);
	}
	void Editable::onFocusLost() {
		Control::onFocusLost();
		setFocused(false);
	}

	void Editable::onMouseMoved(const MouseEvent &e) {
		UIElement::onMouseMoved(e);
		if(_selecting) {
			int pos = getPosAt(e.getPos().x);
			uchar dir = pos > (int)_cursor ? DIR_RIGHT : DIR_LEFT;
			changeSelection(pos,pos,dir);
			moveCursorTo(pos);
			repaint();
		}
	}
	void Editable::onMouseReleased(const MouseEvent &e) {
		UIElement::onMouseReleased(e);
		if(_startSel) {
			moveCursorTo(getPosAt(e.getPos().x));
			clearSelection();
			repaint();
		}
		_startSel = false;
		_selecting = false;
	}
	void Editable::onMousePressed(const MouseEvent &e) {
		UIElement::onMousePressed(e);
		_selecting = true;
		_startSel = true;
	}

	int Editable::getPosAt(gpos_t x) {
		int pos = 0;
		if(x >= 0) {
			pos = x / getGraphics()->getFont().getSize().width;
			if(pos > (int)_str.length())
				pos = _str.length();
		}
		return pos;
	}

	void Editable::insertAtCursor(char c) {
		char str[] = {c,'\0'};
		insertAtCursor(str);
	}

	void Editable::insertAtCursor(const string &str) {
		deleteSelection();
		_str.insert(_cursor,str);
		makeDirty(true);
		moveCursor(str.length());
	}

	void Editable::removeLast() {
		if(_selStart != -1)
			deleteSelection();
		else if(_cursor > 0) {
			_str.erase(_cursor - 1,1);
			makeDirty(true);
			moveCursor(-1);
		}
	}

	void Editable::removeNext() {
		if(_selStart != -1)
			deleteSelection();
		else if(_cursor < _str.length()) {
			_str.erase(_cursor,1);
			makeDirty(true);
		}
	}

	void Editable::moveCursor(int amount) {
		setCursorPos(_cursor + amount);
	}

	void Editable::moveCursorTo(size_t pos) {
		setCursorPos(pos);
	}

	void Editable::clearSelection() {
		makeDirty(_selStart != -1 || _selEnd != -1);
		_selStart = -1;
		_selEnd = -1;
		_selDir = DIR_NONE;
	}

	void Editable::changeSelection(int pos,int oldPos,uchar dir) {
		int oldStart = _selStart;
		int oldEnd = _selEnd;
		if(_startSel || _selStart == -1) {
			_selStart = dir == DIR_RIGHT ? oldPos : pos;
			_selEnd = dir == DIR_LEFT ? oldPos : pos;
			_startSel = false;
			_selDir = dir;
		}
		else {
			if(_selDir == DIR_LEFT) {
				if(pos > _selEnd) {
					_selStart = _selEnd;
					_selEnd = pos;
					_selDir = dir;
				}
				else
					_selStart = pos;
			}
			else {
				if(pos <= _selStart) {
					_selEnd = _selStart;
					_selStart = pos;
					_selDir = dir;
				}
				else
					_selEnd = pos;
			}
		}
		makeDirty(_selStart != oldStart || _selEnd != oldEnd);
	}

	void Editable::deleteSelection() {
		if(_selStart != -1) {
			_str.erase(_selStart,_selEnd - _selStart);
			makeDirty(true);
			moveCursorTo(_selStart);
			clearSelection();
		}
	}

	void Editable::onKeyPressed(const KeyEvent &e) {
		UIElement::onKeyPressed(e);
		if(e.isPrintable())
			insertAtCursor(e.getCharacter());
		else {
			size_t oldPos;
			switch(e.getKeyCode()) {
				case VK_DELETE:
					removeNext();
					break;
				case VK_BACKSP:
					removeLast();
					break;
				case VK_LEFT:
					if(_cursor > 0)
						moveCursor(-1);
					if(e.isShiftDown())
						changeSelection(_cursor,_cursor + 1,DIR_LEFT);
					else if(_selStart != -1)
						clearSelection();
					break;
				case VK_RIGHT:
					if(_cursor < _str.length())
						moveCursor(1);
					if(e.isShiftDown())
						changeSelection(_cursor,_cursor - 1,DIR_RIGHT);
					else if(_selStart != -1)
						clearSelection();
					break;
				case VK_HOME:
					oldPos = _cursor;
					if(_cursor != 0)
						moveCursorTo(0);
					if(e.isShiftDown())
						changeSelection(_cursor,oldPos,DIR_LEFT);
					else if(_selStart != -1)
						clearSelection();
					break;
				case VK_END:
					oldPos = _cursor;
					if(_cursor != _str.length())
						moveCursorTo(_str.length());
					if(e.isShiftDown())
						changeSelection(_cursor,oldPos,DIR_RIGHT);
					else if(_selStart != -1)
						clearSelection();
					break;
			}
		}
		repaint();
	}

	void Editable::paint(Graphics &g) {
		Size fsize = g.getFont().getSize();
		Size size = getSize();
		int count = getMaxCharNum(g);
		gpos_t ystart = (size.height - fsize.height) / 2;
		int start = _begin;

		string text = _secret ? string(_str.length(),'*') : _str;
		count = MIN((int)text.length(),count);

		g.setColor(getTheme().getColor(Theme::TEXT_BACKGROUND));
		g.fillRect(1,1,size.width - 2,size.height - 2);
		g.setColor(getTheme().getColor(_focused ? Theme::CTRL_DARKBORDER : Theme::CTRL_BORDER));
		g.drawRect(Pos(0,0),size);

		gsize_t pad = getTheme().getTextPadding();
		if(_selStart != -1) {
			int spos;
			/* selection background */
			g.setColor(getTheme().getColor(Theme::SEL_BACKGROUND));
			spos = (start > _selStart ? 0 : (_selStart - start));
			g.fillRect(pad + fsize.width * spos,
					ystart - CURSOR_OVERLAP,
					fsize.width * (MIN(count - spos,MIN(_selEnd - start,_selEnd - _selStart))),
					fsize.height + CURSOR_OVERLAP * 2);

			/* part before selection */
			if(start < _selStart) {
				g.setColor(getTheme().getColor(Theme::TEXT_FOREGROUND));
				g.drawString(pad,ystart,text,start,MIN(count,_selStart));
			}
			/* selection */
			g.setColor(getTheme().getColor(Theme::SEL_FOREGROUND));
			g.drawString(pad + fsize.width * spos,ystart,text,MAX(start,_selStart),
					(MIN(count - spos,MIN(_selEnd - start,_selEnd - _selStart))));
			/* part behind selection */
			if(_selEnd < start + count) {
				g.setColor(getTheme().getColor(Theme::TEXT_FOREGROUND));
				spos = _selEnd - start;
				g.drawString(pad + spos * fsize.width,ystart,text,_selEnd,
					MIN(count - spos,MIN((int)text.length() - start,(int)text.length() - _selEnd)));
			}
		}
		else {
			g.setColor(getTheme().getColor(Theme::TEXT_FOREGROUND));
			g.drawString(pad,ystart,text,start,count);
		}

		if(_focused) {
			g.setColor(getTheme().getColor(Theme::CTRL_DARKBORDER));
			g.fillRect(pad + fsize.width * (_cursor - start),
					ystart - CURSOR_OVERLAP,CURSOR_WIDTH,fsize.height + CURSOR_OVERLAP * 2);
		}
	}
}
