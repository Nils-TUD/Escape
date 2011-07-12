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

int mouse_start(void *drvIdPtr) {
	int drvId = *(int*)drvIdPtr;
	int mouse = open("/dev/mouse",IO_READ);
	if(mouse < 0)
		error("Unable to open /dev/mouse");

	while(1) {
		ssize_t count = RETRY(read(mouse,mouseData,sizeof(mouseData)));
		if(count < 0)
			printe("[WINM] Unable to read from mouse");
		else {
			if(!win_isEnabled())
				continue;

			sMouseData *msd = mouseData;
			count /= sizeof(sMouseData);
			while(count-- > 0) {
				handleMouseMessage(drvId,msd);
				msd++;
			}
		}
	}
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
			if(w->style != WIN_STYLE_DESKTOP) {
				if(w)
					win_setActive(w->id,true,curX,curY);
				else
					win_setActive(WINDOW_COUNT,false,curX,curY);
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
			bool left = curY >= w->y + tbh && curX < w->x + CURSOR_RESIZE_WIDTH;
			bool right = curY >= w->y + tbh && curX >= w->x + w->width - CURSOR_RESIZE_WIDTH;
			bool bottom = curY >= w->y + w->height - CURSOR_RESIZE_WIDTH;
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
		int aWin = getClient(drvId,w->owner);
		if(aWin >= 0) {
			msg.args.arg1 = curX;
			msg.args.arg2 = curY;
			msg.args.arg3 = mdata->x;
			msg.args.arg4 = -mdata->y;
			msg.args.arg5 = mdata->z;
			msg.args.arg6 = mdata->buttons;
			msg.args.arg7 = w->id;
			send(aWin,MSG_WIN_MOUSE_EV,&msg,sizeof(msg.args));
			close(aWin);
		}
	}

	if(btnChanged && !buttons)
		mouseWin = NULL;
}
