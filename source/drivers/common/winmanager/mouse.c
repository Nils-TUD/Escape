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
#include <esc/io.h>
#include <esc/driver.h>
#include <esc/messages.h>
#include <stdio.h>
#include <stdlib.h>

#include "mouse.h"
#include "window.h"

#define MOUSE_DATA_BUF_SIZE	128

static void handleMouseMessage(int drvId,sMouseData *mdata);

static sMsg msg;
static uchar buttons = 0;
static gpos_t curX = 0;
static gpos_t curY = 0;
static uchar cursor = CURSOR_DEFAULT;
static sMouseData mouseData[MOUSE_DATA_BUF_SIZE];
static sWindow *mouseWin = NULL;
static int winmng = -1;

int mouse_start(void *drvIdPtr) {
	int drvId = *(int*)drvIdPtr;
	int mouse = open("/dev/mouse",IO_READ);
	if(mouse < 0)
		error("Unable to open /dev/mouse");
	winmng = open("/dev/winmanager",IO_MSGS);
	if(winmng < 0)
		error("Unable to open /dev/winmanager");

	while(1) {
		ssize_t count = IGNSIGS(read(mouse,mouseData,sizeof(mouseData)));
		if(count < 0)
			printe("[WINM] Unable to read from mouse");
		else {
			sMouseData *msd;
			if(!win_isEnabled())
				continue;

			msd = mouseData;
			count /= sizeof(sMouseData);
			while(count-- > 0) {
				handleMouseMessage(drvId,msd);
				msd++;
			}
		}
	}
	close(winmng);
	close(mouse);
	return 0;
}

gpos_t mouse_getX(void) {
	return curX;
}

gpos_t mouse_getY(void) {
	return curY;
}

static void handleMouseMessage(int drvId,sMouseData *mdata) {
	gpos_t oldx = curX,oldy = curY;
	bool btnChanged = false;
	sWindow *w,*wheelWin = NULL;
	curX = MAX(0,MIN(win_getScreenWidth() - 1,curX + mdata->x));
	curY = MAX(0,MIN(win_getScreenHeight() - 1,curY - mdata->y));

	/* set active window */
	if(mdata->buttons != buttons) {
		btnChanged = true;
		buttons = mdata->buttons;
		if(buttons) {
			w = win_getAt(curX,curY);
			if(w) {
				/* do that via message passing so that only one thread performs changes on the
				 * windows */
				msg.args.arg1 = w->id;
				send(winmng,MSG_WIN_SET_ACTIVE,&msg,sizeof(msg.args));
			}
			mouseWin = w;
		}
	}
	else if(mdata->z)
		wheelWin = win_getAt(curX,curY);

	/* if no buttons are pressed, change the cursor if we're at a window-border */
	if(!buttons) {
		w = mouseWin ? mouseWin : win_getAt(curX,curY);
		cursor = CURSOR_DEFAULT;
		if(w && w->style != WIN_STYLE_POPUP && w->style != WIN_STYLE_DESKTOP) {
			gsize_t tbh = w->titleBarHeight;
			bool left = curY >= (gpos_t)(w->y + tbh) &&
					curX < (gpos_t)(w->x + CURSOR_RESIZE_WIDTH);
			bool right = curY >= (gpos_t)(w->y + tbh) &&
					curX >= (gpos_t)(w->x + w->width - CURSOR_RESIZE_WIDTH);
			bool bottom = curY >= (gpos_t)(w->y + w->height - CURSOR_RESIZE_WIDTH);
			if(left && bottom)
				cursor = CURSOR_RESIZE_BL;
			else if(left)
				cursor = CURSOR_RESIZE_L;
			if(right && bottom)
				cursor = CURSOR_RESIZE_BR;
			else if(right)
				cursor = CURSOR_RESIZE_R;
			else if(bottom && !left)
				cursor = CURSOR_RESIZE_VERT;
		}
	}

	/* let vesa draw the cursor */
	if(curX != oldx || curY != oldy)
		win_setCursor(curX,curY,cursor);

	/* send to window */
	w = wheelWin ? wheelWin : (mouseWin ? mouseWin : win_getActive());
	if(w) {
		msg.args.arg1 = curX;
		msg.args.arg2 = curY;
		msg.args.arg3 = mdata->x;
		msg.args.arg4 = -mdata->y;
		msg.args.arg5 = mdata->z;
		msg.args.arg6 = mdata->buttons;
		msg.args.arg7 = w->id;
		send(w->owner,MSG_WIN_MOUSE_EV,&msg,sizeof(msg.args));
	}

	if(btnChanged && !buttons)
		mouseWin = NULL;
}
