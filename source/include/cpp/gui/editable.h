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

#ifndef EDITABLE_H_
#define EDITABLE_H_

#include <esc/common.h>
#include <gui/control.h>
#include <string>

namespace gui {
	class Editable : public Control {
	public:
		static const uchar DIR_NONE;
		static const uchar DIR_LEFT;
		static const uchar DIR_RIGHT;

		static const gsize_t CURSOR_WIDTH	= 2;
		static const gsize_t CURSOR_OVERLAP	= 2;
		static const gsize_t DEF_WIDTH		= 100;

	public:
		Editable()
			: Control(), _cursor(0), _begin(0), _focused(false), _selecting(false),
			  _startSel(false), _selDir(DIR_NONE), _selStart(-1), _selEnd(-1), _str(string()) {
		};
		Editable(gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: Control(x,y,width,height), _cursor(0), _begin(0), _focused(false), _selecting(false),
			  _startSel(false), _selDir(DIR_NONE), _selStart(-1), _selEnd(-1), _str(string()) {
		};
		Editable(const Editable &e)
			: Control(e), _cursor(e._cursor), _begin(0), _focused(false), _selecting(false),
			  _startSel(false), _selDir(DIR_NONE), _selStart(-1), _selEnd(-1), _str(e._str) {
		};
		virtual ~Editable() {
		};
		Editable &operator=(const Editable &e);

		inline string getText() const {
			return _str;
		};
		inline void setText(const string &text) {
			_str = text;
			_cursor = text.length();
			repaint();
		};

		virtual gsize_t getMinWidth() const;
		virtual gsize_t getMinHeight() const;
		virtual void onFocusGained();
		virtual void onFocusLost();
		virtual void onMouseMoved(const MouseEvent &e);
		virtual void onMouseReleased(const MouseEvent &e);
		virtual void onMousePressed(const MouseEvent &e);
		virtual void onKeyPressed(const KeyEvent &e);
		virtual void paint(Graphics &g);

	private:
		int getPosAt(gpos_t x);
		void moveCursor(int amount);
		bool moveCursorTo(size_t pos);
		void clearSelection();
		bool changeSelection(int pos,int oldPos,uchar dir);
		void deleteSelection();
		inline size_t getMaxCharNum(Graphics &g) {
			if(getWidth() < getTheme().getTextPadding() * 2)
				return 0;
			return (getWidth() - getTheme().getTextPadding() * 2) / g.getFont().getWidth();
		};

	private:
		size_t _cursor;
		size_t _begin;
		bool _focused;
		bool _selecting;
		bool _startSel;
		uchar _selDir;
		int _selStart;
		int _selEnd;
		string _str;
	};
}

#endif /* EDITABLE_H_ */
