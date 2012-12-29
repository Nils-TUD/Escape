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
#include <gui/editable.h>
#include <esc/keycodes.h>

namespace gui {
	const uchar Editable::DIR_NONE			= 0;
	const uchar Editable::DIR_LEFT			= 1;
	const uchar Editable::DIR_RIGHT			= 2;

	const gsize_t Editable::CURSOR_WIDTH	= 2;
	const gsize_t Editable::CURSOR_OVERLAP	= 2;
	const gsize_t Editable::DEF_WIDTH		= 100;

	gsize_t Editable::getMinWidth() const {
		return DEF_WIDTH + getTheme().getTextPadding() * 2;
	}
	gsize_t Editable::getMinHeight() const {
		return getGraphics()->getFont().getHeight() + getTheme().getTextPadding() * 2;
	}

	gsize_t Editable::getPreferredWidth() const {
		gsize_t strwidth = getGraphics()->getFont().getStringWidth(_str);
		return _prefWidth ? _prefWidth : max(DEF_WIDTH,strwidth) + getTheme().getTextPadding() * 2;
	}

	void Editable::onFocusGained() {
		Control::onFocusGained();
		_focused = true;
		repaint();
	}
	void Editable::onFocusLost() {
		Control::onFocusLost();
		_focused = false;
		repaint();
	}

	void Editable::onMouseMoved(const MouseEvent &e) {
		UIElement::onMouseMoved(e);
		if(_selecting) {
			bool changed = false;
			int pos = getPosAt(e.getX());
			uchar dir = pos > (int)_cursor ? DIR_RIGHT : DIR_LEFT;
			changed |= changeSelection(pos,pos,dir);
			changed |= moveCursorTo(pos);
			if(changed)
				repaint();
		}
	}
	void Editable::onMouseReleased(const MouseEvent &e) {
		UIElement::onMouseReleased(e);
		if(_startSel) {
			moveCursorTo(getPosAt(e.getX()));
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
			pos = x / getGraphics()->getFont().getWidth();
			if(pos > (int)_str.length())
				pos = _str.length();
		}
		return pos;
	}

	void Editable::moveCursor(int amount) {
		size_t max = getMaxCharNum(*getGraphics());
		_cursor += amount;
		if(amount > 0 && _cursor - _begin > max)
			_begin += amount;
		else if(amount < 0 && _cursor < _begin)
			_begin = _cursor;
	}

	bool Editable::moveCursorTo(size_t pos) {
		size_t oldCur = _cursor;
		size_t max = getMaxCharNum(*getGraphics());
		_cursor = pos;
		if(_cursor > max)
			_begin = _cursor - max;
		else
			_begin = 0;
		return _cursor != oldCur;
	}

	void Editable::clearSelection() {
		_selStart = -1;
		_selEnd = -1;
		_selDir = DIR_NONE;
	}

	bool Editable::changeSelection(int pos,int oldPos,uchar dir) {
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
		return _selStart != oldStart || _selEnd != oldEnd;
	}

	void Editable::deleteSelection() {
		if(_selStart != -1) {
			_str.erase(_selStart,_selEnd - _selStart);
			moveCursorTo(_selStart);
			clearSelection();
		}
	}

	void Editable::onKeyPressed(const KeyEvent &e) {
		UIElement::onKeyPressed(e);
		if(e.isPrintable()) {
			deleteSelection();
			_str.insert(_cursor,1,e.getCharacter());
			moveCursor(1);
			repaint();
		}
		else {
			size_t oldPos;
			bool changed = false;
			switch(e.getKeyCode()) {
				case VK_DELETE:
					if(_selStart != -1) {
						deleteSelection();
						changed = true;
					}
					else if(_cursor < _str.length()) {
						_str.erase(_cursor,1);
						changed = true;
					}
					break;
				case VK_BACKSP:
					if(_selStart != -1) {
						deleteSelection();
						changed = true;
					}
					else if(_cursor > 0) {
						_str.erase(_cursor - 1,1);
						moveCursor(-1);
						changed = true;
					}
					break;
				case VK_LEFT:
					if(_cursor > 0) {
						moveCursor(-1);
						changed = true;
					}
					if(e.isShiftDown())
						changed |= changeSelection(_cursor,_cursor + 1,DIR_LEFT);
					else if(_selStart != -1) {
						clearSelection();
						changed = true;
					}
					break;
				case VK_RIGHT:
					if(_cursor < _str.length()) {
						moveCursor(1);
						changed = true;
					}
					if(e.isShiftDown())
						changed |= changeSelection(_cursor,_cursor - 1,DIR_RIGHT);
					else if(_selStart != -1) {
						clearSelection();
						changed = true;
					}
					break;
				case VK_HOME:
					oldPos = _cursor;
					if(_cursor != 0) {
						moveCursorTo(0);
						changed = true;
					}
					if(e.isShiftDown())
						changed |= changeSelection(_cursor,oldPos,DIR_LEFT);
					else if(_selStart != -1) {
						clearSelection();
						changed = true;
					}
					break;
				case VK_END:
					oldPos = _cursor;
					if(_cursor != _str.length()) {
						moveCursorTo(_str.length());
						changed = true;
					}
					if(e.isShiftDown())
						changed |= changeSelection(_cursor,oldPos,DIR_RIGHT);
					else if(_selStart != -1) {
						clearSelection();
						changed = true;
					}
					break;
			}

			if(changed)
				repaint();
		}
	}

	void Editable::paint(Graphics &g) {
		gsize_t cwidth = g.getFont().getWidth();
		gsize_t cheight = g.getFont().getHeight();
		int count = getMaxCharNum(g);
		gpos_t ystart = (getHeight() - cheight) / 2;
		int start = _begin;
		count = MIN((int)_str.length(),count);

		g.setColor(getTheme().getColor(Theme::TEXT_BACKGROUND));
		g.fillRect(1,1,getWidth() - 2,getHeight() - 2);
		g.setColor(getTheme().getColor(Theme::CTRL_BORDER));
		g.drawRect(0,0,getWidth(),getHeight());

		gsize_t pad = getTheme().getTextPadding();
		if(_selStart != -1) {
			int spos;
			/* selection background */
			g.setColor(getTheme().getColor(Theme::SEL_BACKGROUND));
			spos = (start > _selStart ? 0 : (_selStart - start));
			g.fillRect(pad + cwidth * spos,
					ystart - CURSOR_OVERLAP,
					cwidth * (MIN(count - spos,MIN(_selEnd - start,_selEnd - _selStart))),
					cheight + CURSOR_OVERLAP * 2);

			/* part before selection */
			if(start < _selStart) {
				g.setColor(getTheme().getColor(Theme::TEXT_FOREGROUND));
				g.drawString(pad,ystart,_str,start,MIN(count,_selStart));
			}
			/* selection */
			g.setColor(getTheme().getColor(Theme::SEL_FOREGROUND));
			g.drawString(pad + cwidth * spos,ystart,_str,MAX(start,_selStart),
					(MIN(count - spos,MIN(_selEnd - start,_selEnd - _selStart))));
			/* part behind selection */
			if(_selEnd < start + count) {
				g.setColor(getTheme().getColor(Theme::TEXT_FOREGROUND));
				spos = _selEnd - start;
				g.drawString(pad + spos * cwidth,ystart,_str,_selEnd,
					MIN(count - spos,MIN((int)_str.length() - start,(int)_str.length() - _selEnd)));
			}
		}
		else {
			g.setColor(getTheme().getColor(Theme::TEXT_FOREGROUND));
			g.drawString(pad,ystart,_str,start,count);
		}

		if(_focused) {
			g.setColor(getTheme().getColor(Theme::CTRL_DARKBORDER));
			g.fillRect(pad + cwidth * (_cursor - start),
					ystart - CURSOR_OVERLAP,CURSOR_WIDTH,cheight + CURSOR_OVERLAP * 2);
		}
	}
}
