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

#ifndef SHELLCONTROL_H_
#define SHELLCONTROL_H_

#include <esc/common.h>
#include <esc/ports.h>
#include <esc/gui/common.h>
#include <esc/gui/control.h>
#include <esc/string.h>

using namespace esc;
using namespace esc::gui;

class ShellControl : public Control {
private:
	typedef enum {BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE} eColor;
	static Color COLORS[10];

	static const u32 TAB_WIDTH = 4;
	static const u32 COLUMNS = 80;
	static const u32 HISTORY_SIZE = 200;
	static const u32 PADDING = 3;
	static const u32 CURSOR_WIDTH = 2;
	static const u32 CURSOR_OVERLAP = 2;
	static const Color BGCOLOR;
	static const Color FGCOLOR;
	static const Color BORDER_COLOR;
	static const Color CURSOR_COLOR;

public:
	ShellControl(tCoord x,tCoord y,tSize width,tSize height)
		: Control(x,y,width,height), _color(WHITE << 4 | BLACK), _row(0), _col(0), _cursorCol(0),
			_scrollRows(0), _firstRow(0), _rows(Vector<Vector<char>*>()) {
		// insert first row
		_rows.add(new Vector<char>(COLUMNS * 2));

		// request ports for qemu and bochs
		requestIOPort(0xe9);
		requestIOPort(0x3f8);
		requestIOPort(0x3fd);
	};
	ShellControl(const ShellControl &e)
		: Control(e), _color(e._color), _row(e._row), _col(e._col), _cursorCol(e._cursorCol),
			_scrollRows(0), _firstRow(e._firstRow) {
		_rows = Vector<Vector<char>*>();
		for(u32 i = 0; i < e._rows.size(); i++)
			_rows.add(new Vector<char>(*e._rows[i]));
	};
	virtual ~ShellControl() {
		for(u32 i = 0; i < _rows.size(); i++)
			delete _rows[i];
		releaseIOPort(0xe9);
		releaseIOPort(0x3f8);
		releaseIOPort(0x3fd);
	};

	ShellControl &operator=(const ShellControl &e) {
		// ignore self-assigments
		if(this == &e)
			return *this;
		Control::operator=(e);
		_color = e._color;
		_row = e._row;
		_col = e._col;
		_cursorCol = e._cursorCol;
		_scrollRows = e._scrollRows;
		_firstRow = e._firstRow;
		_rows = Vector<Vector<char>*>();
		for(u32 i = 0; i < e._rows.size(); i++)
			_rows.add(new Vector<char>(*e._rows[i]));
		return *this;
	};

	void append(const char *s);
	void scrollPage(s32 up);
	void scrollLine(s32 up);
	virtual void paint(Graphics &g);

private:
	void append(char c);
	void clearRows(Graphics &g,u32 start,u32 count);
	void paintRows(Graphics &g,u32 start,u32 count);
	inline u32 getLineCount() const {
		return (getHeight() / (getGraphics()->getFont().getHeight() + PADDING));
	};
	bool handleEscape(const char *s);

	u8 _color;
	u32 _row;
	u32 _col;
	u32 _cursorCol;
	u32 _scrollRows;
	u32 _firstRow;
	Vector<Vector<char>*> _rows;
};

#endif /* SHELLCONTROL_H_ */
