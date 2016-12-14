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
#include <sys/conf.h>
#include <sys/esccodes.h>
#include <sys/io.h>
#include <sys/messages.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bar.h"
#include "game.h"
#include "object.h"
#include "objlist.h"
#include "ui.h"

static const uchar airplane[Object::AIRPLANE_WIDTH * Object::AIRPLANE_HEIGHT * 2] = {
	0xDA, 0x07, 0xC4, 0x07, 0xBF, 0x07,
	0xB3, 0x07, 0xDB, 0x07, 0xB3, 0x07,
	0xD4, 0x07, 0xCD, 0x07, 0xBE, 0x07
};

static const uchar explo1[Object::AIRPLANE_WIDTH * Object::AIRPLANE_HEIGHT * 2] = {
	0xDA, 0x07, 0xC4, 0x07, 0xBF, 0x07,
	0xB3, 0x07, 0xB2, 0x0E, 0xB3, 0x07,
	0xD4, 0x07, 0xCD, 0x07, 0xBE, 0x07
};

static const uchar explo2[Object::AIRPLANE_WIDTH * Object::AIRPLANE_HEIGHT * 2] = {
	0xB0, 0x0E, 0xB0, 0x0E, 0xB0, 0x0E,
	0xB0, 0x0E, 0xB0, 0x0E, 0xB0, 0x0E,
	0xB0, 0x0E, 0xB0, 0x0E, 0xB0, 0x0E
};

static const uchar explo3[Object::AIRPLANE_WIDTH * Object::AIRPLANE_HEIGHT * 2] = {
	0xB1, 0x06, 0xB1, 0x06, 0xB1, 0x06,
	0xB1, 0x06, 0xB1, 0x06, 0xB1, 0x06,
	0xB1, 0x06, 0xB1, 0x06, 0xB1, 0x06
};

static const uchar explo4[Object::AIRPLANE_WIDTH * Object::AIRPLANE_HEIGHT * 2] = {
	0xB0, 0x08, 0xB0, 0x08, 0xB0, 0x08,
	0xB0, 0x08, 0xB0, 0x08, 0xB0, 0x08,
	0xB0, 0x08, 0xB0, 0x08, 0xB0, 0x08
};

static const uchar bullet[Object::BULLET_WIDTH * Object::BULLET_HEIGHT * 2] = {
	0x04, 0x04
};

UI::UI(Game &game,uint cols,uint rows) : game(game) {
	/* open uimanager */
	ui = new esc::UI("/dev/uimng");

	/* find desired mode */
	mode = ui->findTextMode(cols,rows);

	/* attach to input-channel */
	uiev = new esc::UIEvents(*ui);

	/* create framebuffer and set mode */
	fb = new esc::FrameBuffer(mode,esc::Screen::MODE_TYPE_TUI);
	ui->setMode(esc::Screen::MODE_TYPE_TUI,mode.id,fb->fd(),true);

	/* start input thread */
	if(startthread(inputThread,this) < 0)
		error("Unable to start input-thread");

	/* create basic screen */
	backup = (uchar*)malloc(width() * height() * 2);
	if(!backup)
		error("Unable to alloc mem for backup");
	setBackup();
}

UI::~UI() {
	if(kill(getpid(),SIGUSR1) < 0)
		printe("Unable to send SIGUSR1");

	delete fb;
	delete uiev;
	delete ui;
	free(backup);
}

static void sigUsr1(A_UNUSED int sig) {
	exit(EXIT_SUCCESS);
}

int UI::inputThread(void *arg) {
	UI *ui = reinterpret_cast<UI*>(arg);

	if(signal(SIGUSR1,sigUsr1) == SIG_ERR)
		error("Unable to set SIGUSR1-handler");

	/* read from uimanager and handle the keys */
	while(1) {
		esc::UIEvents::Event ev;
		*ui->uiev >> ev;
		if(ev.type == esc::UIEvents::Event::TYPE_KEYBOARD)
			ui->game.handleKey(ev.d.keyb.keycode,ev.d.keyb.modifier,ev.d.keyb.character);
	}
	return 0;
}

void UI::update() {
	restoreBackup();
	drawBar(game.bar());
	drawObjects(game.objects());
	drawScore();
	ui->update(0,0,width(),height());
}

void UI::drawScore() {
	char scoreStr[SCORE_WIDTH];
	snprintf(scoreStr,sizeof(scoreStr),"%*u",SCORE_WIDTH - 2,game.getScore());
	for(size_t i = 0, x = width() - SCORE_WIDTH + 1; scoreStr[i]; i++, x++)
		backup[xyChar(x,3)] = scoreStr[i];
}

void UI::drawObjects(const ObjList &objlist) {
	for(Object *o = objlist.get(); o != NULL; o = o->next()) {
		if((size_t)(o->x + PADDING + o->width) >= (size_t)(width() - SCORE_WIDTH) &&
				(o->y + PADDING) <= SCORE_HEIGHT) {
			/* don't draw objects over the score-area */
			continue;
		}

		const uchar *src;
		switch(o->type) {
			case Object::AIRPLANE:
				src = airplane;
				break;
			case Object::BULLET:
				src = bullet;
				break;
			case Object::EXPLO1:
				src = explo1;
				break;
			case Object::EXPLO2:
				src = explo2;
				break;
			case Object::EXPLO3:
				src = explo3;
				break;
			case Object::EXPLO4:
			default:
				src = explo4;
				break;
		}

		for(uint y = o->y + PADDING; y < o->y + PADDING + o->height; y++) {
			memcpy(fb->addr() + xyChar(o->x + PADDING,y),src,o->width * 2);
			src += o->width * 2;
		}
	}
}

void UI::drawBar(const Bar &bar) {
	size_t start,end;
	bar.getDim(&start,&end);

	for(size_t x = start + PADDING; x <= end; x++) {
		fb->addr()[xyChar(x,height() - 2)] = 0xDB;
		fb->addr()[xyCol(x,height() - 2)] = 0x07;
	}
}

void UI::restoreBackup() {
	memcpy(fb->addr(),backup,width() * height() * 2);
}

void UI::setBackup() {
	/* fill bg */
	for(size_t i = 0; i < width() * height() * 2; i += 2) {
		backup[i] = ' ';
		backup[i + 1] = 0x07;
	}

	/* top and bottom border */
	for(size_t x = 1; x < width() - 1; x++) {
		backup[xyChar(x,0)] = 0xCD;
		backup[xyCol(x,0)] = 0x07;
		backup[xyChar(x,height() - 1)] = 0xCD;
		backup[xyCol(x,height() - 1)] = 0x07;
	}

	/* left and right border */
	for(size_t y = 1; y < height() - 1; y++) {
		backup[xyChar(0,y)] = 0xBA;
		backup[xyCol(0,y)] = 0x07;
		backup[xyChar(width() - 1,y)] = 0xBA;
		backup[xyCol(width() - 1,y)] = 0x07;
	}

	/* corners */
	backup[xyChar(0,0)] = 0xC9;
	backup[xyCol(0,0)] = 0x07;
	backup[xyChar(width() - 1,0)] = 0xBB;
	backup[xyCol(width() - 1,0)] = 0x07;
	backup[xyChar(0,height() - 1)] = 0xC8;
	backup[xyCol(0,height() - 1)] = 0x07;
	backup[xyChar(width() - 1,height() - 1)] = 0xBC;
	backup[xyCol(width() - 1,height() - 1)] = 0x07;

	/* score-border */
	backup[xyChar(width() - SCORE_WIDTH,0)] = 0xCB;
	for(size_t y = 1; y < SCORE_HEIGHT; y++)
		backup[xyChar(width() - SCORE_WIDTH,y)] = 0xBA;
	backup[xyChar(width() - SCORE_WIDTH,SCORE_HEIGHT)] = 0xC8;
	for(size_t x = width() - SCORE_WIDTH + 1; x < width() - 1; x++)
		backup[xyChar(x,SCORE_HEIGHT)] = 0xCD;
	backup[xyChar(width() - 1,SCORE_HEIGHT)] = 0xB9;

	/* "Score:" */
	const char *title = "Score:";
	for(size_t x = width() - SCORE_WIDTH + 1; *title; x++)
		backup[xyChar(x,1)] = *title++;
}
