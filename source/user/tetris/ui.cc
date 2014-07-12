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
#include <sys/thread.h>
#include <sys/keycodes.h>
#include <stdlib.h>

#include "grid.h"
#include "game.h"
#include "ui.h"

UI::UI(int c,int r,int size)
		: _ui("/dev/uimng"), _mode(_ui.findTextMode(c,r)), _uievents(_ui), _fb(),
		  _cpp(size == 1 ? 2 : 3), _rpp(size == 1 ? 1 : 2), _run(true) {
	if(c < minCols() || r < minRows())
		VTHROW("Too few columns. I need at least " << minCols() << "x" << minRows());

	// create shm
	char shmname[SSTRLEN("tetris") + 12];
	for(int id = 1; _fb == NULL; ++id) {
		snprintf(shmname,sizeof(shmname),"tetris%d",id);
		try {
			_fb = new ipc::FrameBuffer(_mode,shmname,ipc::Screen::MODE_TYPE_TUI,0644);
		}
		catch(...) {
		}
	}

	_ui.setMode(ipc::Screen::MODE_TYPE_TUI,_mode.id,shmname,true);
}

UI::~UI() {
	delete _fb;
}

void UI::paintGrid(Grid &grid) {
	int xstart = gridStartx();
	int ystart = gridStarty();
	for(int x = 0; x < gridCols(); ++x) {
		for(int y = 0; y < gridRows(); ++y) {
			char col = grid.get(x,y);
			paintBox(xstart + x * _cpp,ystart + y * _rpp,(col << 4) | BLACK);
		}
	}
}

void UI::paintBox(int px,int py,char color) {
	for(int x = 0; x < _cpp; ++x) {
		for(int y = 0; y < _rpp; ++y)
			set(px + x,py + y,' ',color);
	}
}

void UI::paintBorder() {
	// fill bg
	char *scr = fb();
	for(int i = 0; i < cols() * rows() * 2; i += 2) {
		scr[i] = ' ';
		scr[i + 1] = COLOR;
	}

	int xstart = gridStartx();
	int ystart = gridStarty();

	paintRect(xstart - 1,ystart - 1,INFO_COLS + gridCols() * _cpp + 2,gridRows() * _rpp + 2,0x07);

	// middle
	for(int y = ystart; y < ystart + gridRows() * _rpp; y++)
		set(xstart + gridCols() * _cpp,y,0xBA,COLOR);
	set(xstart + gridCols() * _cpp,ystart - 1,0xCB,COLOR);
	set(xstart + gridCols() * _cpp,ystart + gridRows() * _rpp,0xCA,COLOR);

	// score separation
	for(int x = xstart + gridCols() * _cpp + 1; x < xstart + INFO_COLS + gridCols() * _cpp; x++)
		set(x,ystart + SEP_OFF,0xCD,COLOR);
	set(xstart + gridCols() * _cpp,ystart + SEP_OFF,0xCC,COLOR);
	set(xstart + INFO_COLS + gridCols() * _cpp,ystart + SEP_OFF,0xB9,COLOR);

	paintInfo(NULL,1,0,0);
}

void UI::paintRect(int posx,int posy,int width,int height,char color) {
	// top and bottom border
	for(int x = posx; x < posx + width; x++) {
		set(x,posy,0xCD,color);
		set(x,posy + height - 1,0xCD,color);
	}
	// left and right border
	for(int y = posy; y < posy + height; y++) {
		set(posx,y,0xBA,color);
		set(posx + width - 1,y,0xBA,color);
	}

	set(posx,posy,0xC9,color);
	set(posx + width - 1,posy,0xBB,color);
	set(posx,posy + height - 1,0xC8,color);
	set(posx + width - 1,posy + height - 1,0xBC,color);
}

void UI::paintInfo(Stone *next,int level,int score,int lines) {
	int xstart = gridStartx();
	int ystart = gridStarty();

	// clear old next-stone
	if(next) {
		int nexty = ystart + NEXT_OFF;
		int nextx = xstart + gridCols() * _cpp + (INFO_COLS / 2) - 4;
		for(int y = 0; y < 4; ++y) {
			for(int x = 0; x < 4; ++x)
				paintBox(nextx + x * _cpp,nexty + y * _rpp,(BLACK << 4) | BLACK);
		}

		// paint new next-stone
		nextx = xstart + gridCols() * _cpp + (INFO_COLS / 2) - next->size();
		for(int y = 0; y < next->size(); ++y) {
			for(int x = 0; x < next->size(); ++x) {
				if(next->get(x,y))
					paintBox(nextx + x * _cpp,nexty + y * _rpp,(next->color() << 4) | BLACK);
			}
		}
	}

	char *s,text[64];
	snprintf(text,sizeof(text),"Level: %6d",level);
	s = text;
	for(int x = xstart + gridCols() * _cpp + (INFO_COLS - strlen(text)) / 2; *s; x++)
		set(x,ystart + LEVEL_OFF,*s++,COLOR);

	snprintf(text,sizeof(text),"Score: %6d",score);
	s = text;
	for(int x = xstart + gridCols() * _cpp + (INFO_COLS - strlen(text)) / 2; *s; x++)
		set(x,ystart + SCORE_OFF,*s++,COLOR);

	snprintf(text,sizeof(text),"Lines: %6d",lines);
	s = text;
	for(int x = xstart + gridCols() * _cpp + (INFO_COLS - strlen(text)) / 2; *s; x++)
		set(x,ystart + LINES_OFF,*s++,COLOR);
}

void UI::paintMsgBox(const char **lines,size_t count) {
	int boxrows = count + 4;
	int xstart = gridStartx() + (gridCols() * _cpp - MSGBOX_COLS) / 2;
	int ystart = (rows() - boxrows) / 2;

	for(int x = xstart + 1; x < xstart + MSGBOX_COLS - 1; ++x) {
		for(int y = ystart + 1; y < ystart + boxrows - 1; ++y)
			set(x,y,' ',COLOR);
	}
	paintRect(xstart,ystart,MSGBOX_COLS,boxrows,COLOR);

	for(size_t i = 0; i < count; ++i) {
		const char *s = lines[i];
		for(int x = xstart + (MSGBOX_COLS - strlen(s)) / 2; *s; x++)
			set(x,ystart + 2 + i,*s++,COLOR);
	}
}

void UI::start() {
	// start input thread
	int tid;
	if((tid = startthread(run,this)) < 0)
		error("Unable to start input-thread");

	while(_run) {
		ipc::UIEvents::Event ev;
		_uievents >> ev;
		switch(ev.type) {
			case ipc::UIEvents::Event::TYPE_KEYBOARD:
			case ipc::UIEvents::Event::TYPE_UI_ACTIVE:
			case ipc::UIEvents::Event::TYPE_UI_INACTIVE:
				Game::handleKey(ev);
				break;

			default:
				break;
		}
	}

	// first wait until the thread is done (we can't remove the resources before that)
	join(tid);
}

int UI::run(void *arg) {
	UI *ui = reinterpret_cast<UI*>(arg);
	while(ui->_run) {
		Game::tick();
		sleep(100);
	}
	return 0;
}
