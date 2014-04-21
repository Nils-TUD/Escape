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

#include <esc/common.h>
#include <esc/io.h>
#include <esc/conf.h>
#include <esc/messages.h>
#include <esc/mem.h>
#include <esc/thread.h>
#include <esc/esccodes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "ui.h"
#include "bar.h"
#include "object.h"
#include "objlist.h"
#include "game.h"

#define SCORE_WIDTH			10
#define SCORE_HEIGHT		4

#define XYCHAR(x,y)			((y) * WIDTH * 2 + (x) * 2)
#define XYCOL(x,y)			((y) * WIDTH * 2 + (x) * 2 + 1)

static int ui_inputThread(void *arg);
static void ui_drawScore(void);
static void ui_drawObjects(void);
static void ui_drawBar(void);
static void ui_restoreBackup(void);
static void ui_setBackup(void);

static const uchar airplane[AIRPLANE_WIDTH * AIRPLANE_HEIGHT * 2] = {
	0xDA, 0x07, 0xC4, 0x07, 0xBF, 0x07,
	0xB3, 0x07, 0xDB, 0x07, 0xB3, 0x07,
	0xD4, 0x07, 0xCD, 0x07, 0xBE, 0x07
};

static const uchar explo1[AIRPLANE_WIDTH * AIRPLANE_HEIGHT * 2] = {
	0xDA, 0x07, 0xC4, 0x07, 0xBF, 0x07,
	0xB3, 0x07, 0xB2, 0x0E, 0xB3, 0x07,
	0xD4, 0x07, 0xCD, 0x07, 0xBE, 0x07
};

static const uchar explo2[AIRPLANE_WIDTH * AIRPLANE_HEIGHT * 2] = {
	0xB0, 0x0E, 0xB0, 0x0E, 0xB0, 0x0E,
	0xB0, 0x0E, 0xB0, 0x0E, 0xB0, 0x0E,
	0xB0, 0x0E, 0xB0, 0x0E, 0xB0, 0x0E
};

static const uchar explo3[AIRPLANE_WIDTH * AIRPLANE_HEIGHT * 2] = {
	0xB1, 0x06, 0xB1, 0x06, 0xB1, 0x06,
	0xB1, 0x06, 0xB1, 0x06, 0xB1, 0x06,
	0xB1, 0x06, 0xB1, 0x06, 0xB1, 0x06
};

static const uchar explo4[AIRPLANE_WIDTH * AIRPLANE_HEIGHT * 2] = {
	0xB0, 0x08, 0xB0, 0x08, 0xB0, 0x08,
	0xB0, 0x08, 0xB0, 0x08, 0xB0, 0x08,
	0xB0, 0x08, 0xB0, 0x08, 0xB0, 0x08
};

static const uchar bullet[BULLET_WIDTH * BULLET_HEIGHT * 2] = {
	0x04, 0x04
};

ipc::Screen::Mode mode;
static ipc::UI *ui;
static ipc::UIEvents *uiev;
static ipc::FrameBuffer *fb;
static uchar *backup = NULL;

void ui_init(uint cols,uint rows) {
	/* open uimanager */
	ui = new ipc::UI("/dev/uimng");

	/* find desired mode */
	mode = ui->findTextMode(cols,rows);

	/* attach to input-channel */
	uiev = new ipc::UIEvents(*ui);

	/* create shm */
	int id = 0;
	char shmname[SSTRLEN("raptor") + 12];
	do {
		snprintf(shmname,sizeof(shmname),"raptor%d",++id);
		try {
			fb = new ipc::FrameBuffer(mode,shmname,ipc::Screen::MODE_TYPE_TUI,0644);
		}
		catch(...) {
		}
	}
	while(fb == NULL);

	/* set mode */
	ui->setMode(ipc::Screen::MODE_TYPE_TUI,mode.id,shmname,true);

	/* start input thread */
	if(startthread(ui_inputThread,NULL) < 0)
		error("Unable to start input-thread");

	/* create basic screen */
	backup = (uchar*)malloc(WIDTH * HEIGHT * 2);
	if(!backup)
		error("Unable to alloc mem for backup");
	ui_setBackup();
}

static void sigUsr1(A_UNUSED int sig) {
	exit(EXIT_SUCCESS);
}

static int ui_inputThread(A_UNUSED void *arg) {
	if(signal(SIG_USR1,sigUsr1) == SIG_ERR)
		error("Unable to set SIG_USR1-handler");
	/* read from uimanager and handle the keys */
	while(1) {
		ipc::UIEvents::Event ev;
		*uiev >> ev;
		if(ev.type == ipc::UIEvents::Event::TYPE_KEYBOARD)
			game_handleKey(ev.d.keyb.keycode,ev.d.keyb.modifier,ev.d.keyb.character);
	}
	return 0;
}

void ui_destroy(void) {
	if(kill(getpid(),SIG_USR1) < 0)
		printe("Unable to send SIG_USR1");
	delete fb;
	delete uiev;
	delete ui;
	free(backup);
}

void ui_update(void) {
	ui_restoreBackup();
	ui_drawBar();
	ui_drawObjects();
	ui_drawScore();
	ui->update(0,0,WIDTH,HEIGHT);
}

static void ui_drawScore(void) {
	size_t x,i;
	char scoreStr[SCORE_WIDTH];
	snprintf(scoreStr,sizeof(scoreStr),"%*u",SCORE_WIDTH - 2,game_getScore());
	for(i = 0, x = WIDTH - SCORE_WIDTH + 1; scoreStr[i]; i++, x++)
		backup[XYCHAR(x,3)] = scoreStr[i];
}

static void ui_drawObjects(void) {
	int y;
	sSLNode *n;
	sObject *o;
	const uchar *src;
	sSLList *objects = objlist_get();
	for(n = sll_begin(objects); n != NULL; n = n->next) {
		o = (sObject*)n->data;
		if((size_t)(o->x + PADDING + o->width) >= (size_t)(WIDTH - SCORE_WIDTH) &&
				(o->y + PADDING) <= SCORE_HEIGHT) {
			/* don't draw objects over the score-area */
			continue;
		}

		switch(o->type) {
			case TYPE_AIRPLANE:
				src = airplane;
				break;
			case TYPE_BULLET:
				src = bullet;
				break;
			case TYPE_EXPLO1:
				src = explo1;
				break;
			case TYPE_EXPLO2:
				src = explo2;
				break;
			case TYPE_EXPLO3:
				src = explo3;
				break;
			case TYPE_EXPLO4:
			default:
				src = explo4;
				break;
		}

		for(y = o->y + PADDING; y < o->y + PADDING + o->height; y++) {
			memcpy(fb->addr() + XYCHAR(o->x + PADDING,y),src,o->width * 2);
			src += o->width * 2;
		}
	}
}

static void ui_drawBar(void) {
	size_t x,start,end;
	bar_getDim(&start,&end);
	for(x = start + PADDING; x <= end; x++) {
		fb->addr()[XYCHAR(x,HEIGHT - 2)] = 0xDB;
		fb->addr()[XYCOL(x,HEIGHT - 2)] = 0x07;
	}
}

static void ui_restoreBackup(void) {
	memcpy(fb->addr(),backup,WIDTH * HEIGHT * 2);
}

static void ui_setBackup(void) {
	size_t i,x,y;
	const char *title = "Score:";
	/* fill bg */
	for(i = 0; i < WIDTH * HEIGHT * 2; i += 2) {
		backup[i] = ' ';
		backup[i + 1] = 0x07;
	}
	/* top and bottom border */
	for(x = 1; x < WIDTH - 1; x++) {
		backup[XYCHAR(x,0)] = 0xCD;
		backup[XYCOL(x,0)] = 0x07;
		backup[XYCHAR(x,HEIGHT - 1)] = 0xCD;
		backup[XYCOL(x,HEIGHT - 1)] = 0x07;
	}
	/* left and right border */
	for(y = 1; y < HEIGHT - 1; y++) {
		backup[XYCHAR(0,y)] = 0xBA;
		backup[XYCOL(0,y)] = 0x07;
		backup[XYCHAR(WIDTH - 1,y)] = 0xBA;
		backup[XYCOL(WIDTH - 1,y)] = 0x07;
	}
	/* corners */
	backup[XYCHAR(0,0)] = 0xC9;
	backup[XYCOL(0,0)] = 0x07;
	backup[XYCHAR(WIDTH - 1,0)] = 0xBB;
	backup[XYCOL(WIDTH - 1,0)] = 0x07;
	backup[XYCHAR(0,HEIGHT - 1)] = 0xC8;
	backup[XYCOL(0,HEIGHT - 1)] = 0x07;
	backup[XYCHAR(WIDTH - 1,HEIGHT - 1)] = 0xBC;
	backup[XYCOL(WIDTH - 1,HEIGHT - 1)] = 0x07;

	/* score-border */
	backup[XYCHAR(WIDTH - SCORE_WIDTH,0)] = 0xCB;
	for(y = 1; y < SCORE_HEIGHT; y++)
		backup[XYCHAR(WIDTH - SCORE_WIDTH,y)] = 0xBA;
	backup[XYCHAR(WIDTH - SCORE_WIDTH,SCORE_HEIGHT)] = 0xC8;
	for(x = WIDTH - SCORE_WIDTH + 1; x < WIDTH - 1; x++)
		backup[XYCHAR(x,SCORE_HEIGHT)] = 0xCD;
	backup[XYCHAR(WIDTH - 1,SCORE_HEIGHT)] = 0xB9;

	/* "Score:" */
	for(x = WIDTH - SCORE_WIDTH + 1; *title; x++)
		backup[XYCHAR(x,1)] = *title++;
}
