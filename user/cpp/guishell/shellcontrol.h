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
#include <ringbuffer.h>
#include <esccodes.h>

using namespace esc;
using namespace esc::gui;

class ShellApplication;

class ShellControl : public Control {
	friend class ShellApplication;

private:
	typedef enum {BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE} eColor;
	static Color COLORS[16];

	static const u32 TAB_WIDTH = 4;
	static const u32 COLUMNS = 80;
	static const u32 ROWS = 25;
	static const u32 HISTORY_SIZE = 200;
	static const u32 PADDING = 3;
	static const u32 CURSOR_WIDTH = 2;
	static const u32 CURSOR_OVERLAP = 2;
	static const u32 INITIAL_RLBUF_SIZE = 50;
	static const u32 RLBUF_INCR = 20;
	static const u32 GUISH_INBUF_SIZE = 128;
	static const Color BGCOLOR;
	static const Color FGCOLOR;
	static const Color BORDER_COLOR;
	static const Color CURSOR_COLOR;

public:
	ShellControl(tServ sid,tCoord x,tCoord y,tSize width,tSize height)
		: Control(x,y,width,height), _sid(sid), _color(WHITE << 4 | BLACK),
			_row(0), _col(0), _cursorCol(0),
			_scrollRows(0), _firstRow(0), _navigation(true), _rlStartCol(0),
			_rlBufSize(INITIAL_RLBUF_SIZE), _rlBufPos(0), _readline(true), _echo(true),
			_escapePos(-1), _backupRow(0), _backupCol(0), _screenBackup(NULL),
			_rows(Vector<Vector<char>*>()) {
		// insert first row
		_rows.add(new Vector<char>(COLUMNS * 2));

		_rlBuffer = new char[_rlBufSize];

		// create input-buffer
		_inbuf = rb_create(sizeof(char),GUISH_INBUF_SIZE,RB_OVERWRITE);
		if(_inbuf == NULL) {
			printe("Unable to create ring-buffer");
			exit(EXIT_FAILURE);
		}

		// open speaker
		_speaker = open("/services/speaker",IO_WRITE);
		if(_speaker < 0) {
			printe("Unable to open '/services/speaker'");
			exit(EXIT_FAILURE);
		}

		// request ports for qemu and bochs
		requestIOPort(0xe9);
		requestIOPort(0x3f8);
		requestIOPort(0x3fd);
	};
	ShellControl(const ShellControl &e) : Control(e) {
		clone(e);
	};
	virtual ~ShellControl() {
		for(u32 i = 0; i < _rows.size(); i++)
			delete _rows[i];
		delete _rlBuffer;
		rb_destroy(_inbuf);
		close(_speaker);
		releaseIOPort(0xe9);
		releaseIOPort(0x3f8);
		releaseIOPort(0x3fd);
	};

	ShellControl &operator=(const ShellControl &e) {
		// ignore self-assigments
		if(this == &e)
			return *this;
		Control::operator=(e);
		clone(e);
		return *this;
	};

	void append(char *s,u32 len);
	void scrollPage(s32 up);
	void scrollLine(s32 up);
	virtual void paint(Graphics &g);
	inline bool getReadLine() {
		return _readline;
	};
	inline bool getEcho() {
		return _echo;
	};
	inline bool getNavigation() {
		return _navigation;
	};

private:
	void append(char c);
	void clearRows(Graphics &g,u32 start,u32 count);
	void paintRows(Graphics &g,u32 start,u32 count);
	inline u32 getLineCount() const {
		return (getHeight() / (getGraphics()->getFont().getHeight() + PADDING));
	};
	inline sRingBuf *getInBuf() {
		return _inbuf;
	};
	bool handleEscape(char **s);
	bool rlHandleKeycode(u8 keycode);
	void rlPutchar(char c);
	u32 rlGetBufPos();
	void rlFlushBuf();
	void addToInBuf(char *s,u32 len);
	s32 ioctl(u32 cmd,void *data,bool *readKb);

	void clone(const ShellControl &e) {
		_sid = e._sid;
		_color = e._color;
		_row = e._row;
		_col = e._col;
		_cursorCol = e._cursorCol;
		_scrollRows = e._scrollRows;
		_firstRow = e._firstRow;
		_navigation = e._navigation;
		_escapePos = e._escapePos;
		_speaker = e._speaker;
		memcpy(_escapeBuf,e._escapeBuf,sizeof(e._escapeBuf));
		_rlStartCol = e._rlStartCol;
		_rlBufSize = e._rlBufSize;
		_rlBufPos = e._rlBufPos;
		_rlBuffer = new char[_rlBufSize];
		_readline = e._readline;
		_echo = e._echo;
		memcpy(_rlBuffer,e._rlBuffer,_rlBufSize);
		_screenBackup = new Vector<Vector<char>*>();
		_backupRow = e._backupRow;
		_backupCol = e._backupCol;
		for(u32 i = 0; i < e._screenBackup->size(); i++)
			_screenBackup->add(new Vector<char>(*((*e._screenBackup)[i])));
		_rows = Vector<Vector<char>*>();
		for(u32 i = 0; i < e._rows.size(); i++)
			_rows.add(new Vector<char>(*e._rows[i]));
	};

	tServ _sid;
	u8 _color;
	u32 _row;
	u32 _col;
	u32 _cursorCol;
	u32 _scrollRows;
	u32 _firstRow;
	bool _navigation;
	u8 _rlStartCol;
	u32 _rlBufSize;
	u32 _rlBufPos;
	char *_rlBuffer;
	bool _readline;
	bool _echo;
	sRingBuf *_inbuf;
	// the escape-state
	s32 _escapePos;
	tFD _speaker;
	u32 _backupRow;
	u32 _backupCol;
	char _escapeBuf[MAX_ESCC_LENGTH];
	Vector<Vector<char>*> *_screenBackup;
	Vector<Vector<char>*> _rows;
};

#endif /* SHELLCONTROL_H_ */
