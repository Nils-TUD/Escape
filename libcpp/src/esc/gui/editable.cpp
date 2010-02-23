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
#include <esc/gui/editable.h>
#include <esc/keycodes.h>

namespace esc {
	namespace gui {
		const u8 Editable::DIR_NONE = 0;
		const u8 Editable::DIR_LEFT = 1;
		const u8 Editable::DIR_RIGHT = 2;
		const Color Editable::BGCOLOR = Color(0xFF,0xFF,0xFF);
		const Color Editable::FGCOLOR = Color(0,0,0);
		const Color Editable::SEL_BGCOLOR = Color(0,0,0xFF);
		const Color Editable::SEL_FGCOLOR = Color(0xFF,0xFF,0xFF);
		const Color Editable::BORDER_COLOR = Color(0,0,0);
		const Color Editable::CURSOR_COLOR = Color(0,0,0);

		Editable &Editable::operator=(const Editable &e) {
			// ignore self-assigments
			if(this == &e)
				return *this;
			Control::operator=(e);
			_cursor = e._cursor;
			_begin = e._begin;
			_focused = false;
			_selecting = false;
			_startSel = false;
			_selDir = e._selDir;
			_selStart = e._selStart;
			_selEnd = e._selEnd;
			_str = e._str;
			return *this;
		}

		void Editable::paint(Graphics &g) {
			u32 cwidth = g.getFont().getWidth();
			u32 cheight = g.getFont().getHeight();
			s32 count = getMaxCharNum(g);
			tCoord ystart = (getHeight() - cheight) / 2;
			s32 start = _begin;
			count = MIN((s32)_str.length(),count);

			g.setColor(BGCOLOR);
			g.fillRect(1,1,getWidth() - 2,getHeight() - 2);
			g.setColor(BORDER_COLOR);
			g.drawRect(0,0,getWidth(),getHeight());

			if(_selStart != -1) {
				s32 spos;
				/* selection background */
				g.setColor(SEL_BGCOLOR);
				spos = (start > _selStart ? 0 : (_selStart - start));
				g.fillRect(PADDING + cwidth * spos,
						ystart - CURSOR_OVERLAP,
						cwidth * (MIN(count - spos,MIN(_selEnd - start,_selEnd - _selStart))),
						cheight + CURSOR_OVERLAP * 2);

				/* part before selection */
				if(start < _selStart) {
					g.setColor(FGCOLOR);
					g.drawString(PADDING,ystart,_str,start,MIN(count,_selStart));
				}
				/* selection */
				g.setColor(SEL_FGCOLOR);
				g.drawString(PADDING + cwidth * spos,ystart,_str,MAX(start,_selStart),
						(MIN(count - spos,MIN(_selEnd - start,_selEnd - _selStart))));
				/* part behind selection */
				if(_selEnd < start + count) {
					g.setColor(FGCOLOR);
					spos = _selEnd - start;
					g.drawString(PADDING + spos * cwidth,ystart,_str,_selEnd,
						MIN(count - spos,MIN((s32)_str.length() - start,(s32)_str.length() - _selEnd)));
				}
			}
			else {
				g.setColor(FGCOLOR);
				g.drawString(PADDING,ystart,_str,start,count);
			}

			if(_focused) {
				g.setColor(CURSOR_COLOR);
				g.fillRect(PADDING + cwidth * (_cursor - start),
						ystart - CURSOR_OVERLAP,CURSOR_WIDTH,cheight + CURSOR_OVERLAP * 2);
			}
		}

		void Editable::onFocusGained() {
			_focused = true;
			repaint();
		}
		void Editable::onFocusLost() {
			_focused = false;
			repaint();
		}

		void Editable::onMouseMoved(const MouseEvent &e) {
			UIElement::onMouseMoved(e);
			if(_selecting) {
				s32 pos = getPosAt(e.getX());
				u8 dir = pos > (s32)_cursor ? DIR_RIGHT : DIR_LEFT;
				changeSelection(pos,pos,dir);
				moveCursorTo(pos);
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

		s32 Editable::getPosAt(tCoord x) {
			s32 pos = 0;
			if(x >= getX()) {
				pos = (x - getX()) / getGraphics()->getFont().getWidth();
				if(pos > (s32)_str.length())
					pos = _str.length();
			}
			return pos;
		}

		void Editable::moveCursor(s32 amount) {
			u32 max = getMaxCharNum(*getGraphics());
			_cursor += amount;
			if(amount > 0 && _cursor - _begin > max)
				_begin += amount;
			else if(amount < 0 && _cursor < _begin)
				_begin = _cursor;
		}

		void Editable::moveCursorTo(u32 pos) {
			u32 max = getMaxCharNum(*getGraphics());
			_cursor = pos;
			if(_cursor > max)
				_begin = _cursor - max;
			else
				_begin = 0;
		}

		void Editable::clearSelection() {
			_selStart = -1;
			_selEnd = -1;
			_selDir = DIR_NONE;
		}

		void Editable::changeSelection(s32 pos,s32 oldPos,u8 dir) {
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
						_selStart = pos;
						_selEnd = _selStart;
						_selDir = dir;
					}
					else
						_selEnd = pos;
				}
			}
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
				_str.insert(_cursor,e.getCharacter());
				moveCursor(1);
				repaint();
			}
			else {
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
					case VK_LEFT:
						if(_cursor > 0) {
							moveCursor(-1);
							if(e.isShiftDown())
								changeSelection(_cursor,_cursor + 1,DIR_LEFT);
							else
								clearSelection();
							changed = true;
						}
						break;
					case VK_RIGHT:
						if(_cursor < _str.length()) {
							moveCursor(1);
							if(e.isShiftDown())
								changeSelection(_cursor,_cursor - 1,DIR_RIGHT);
							else
								clearSelection();
							changed = true;
						}
						break;
					case VK_HOME:
						if(_cursor > 0) {
							u32 oldPos = _cursor;
							moveCursorTo(0);
							if(e.isShiftDown())
								changeSelection(_cursor,oldPos,DIR_LEFT);
							else
								clearSelection();
							changed = true;
						}
						break;
					case VK_END:
						if(_cursor < _str.length()) {
							u32 oldPos = _cursor;
							moveCursorTo(_str.length());
							if(e.isShiftDown())
								changeSelection(_cursor,oldPos,DIR_RIGHT);
							else
								clearSelection();
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
				}

				if(changed)
					repaint();
			}
		}
	}
}
