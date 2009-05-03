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
#include <esc/rect.h>
#include <esc/mem.h>
#include <esc/fileio.h>
#include <esc/heap.h>
#include <esc/proc.h>
#include <esc/service.h>
#include <esc/io.h>
#include <stdlib.h>
#include <sllist.h>
#include <string.h>
#include "window.h"

typedef struct {
	sMsgHeader header;
	sMsgDataWinRepaint data;
} __attribute__((packed)) sMsgRepaint;

typedef struct {
	sMsgHeader header;
	sMsgDataVesaUpdate data;
} __attribute__((packed)) sMsgVesaUpdate;

typedef struct {
	sMsgHeader header;
	sMsgDataVesaCursor data;
} __attribute__((packed)) sMsgVesaCursor;

static sMsgVesaUpdate vesaMsg = {
	.header = {
		.id = MSG_VESA_UPDATE,
		.length = sizeof(sMsgDataVesaUpdate)
	}
};
static sMsgRepaint repaintMsg = {
	.header = {
		.id = MSG_WIN_REPAINT,
		.length = sizeof(sMsgDataWinRepaint)
	}
};
static sMsgVesaCursor cursorMsg = {
	.header = {
		.id = MSG_VESA_CURSOR,
		.length = sizeof(sMsgDataVesaCursor)
	}
};

static void win_repaint(sRectangle *r,sWindow *win,s16 z);
static void win_getRepaintRegions(sSLList *list,tWinId id,sWindow *win,s16 z,sRectangle *r);
static void win_clearRegion(u8 *mem,tCoord x,tCoord y,tSize width,tSize height);
static void win_notifyVesa(tCoord x,tCoord y,tSize width,tSize height);

static tFD vesa;
static tServ servId;
static tSize screenWidth;
static tSize screenHeight;
static u8 colorDepth;

static u8 *shmem;
static u16 activeWindow = WINDOW_COUNT;
static sWindow windows[WINDOW_COUNT];

bool win_init(tServ sid) {
	sMsgHeader header;

	servId = sid;

	/* mark windows unused */
	tWinId i;
	for(i = 0; i < WINDOW_COUNT; i++)
		windows[i].id = WINID_UNSED;

	vesa = open("services:/vesa",IO_WRITE);
	if(vesa < 0) {
		printe("Unable to open services:/vesa");
		return EXIT_FAILURE;
	}

	/* request screen infos from vesa */
	header.id = MSG_VESA_GETMODE_REQ;
	header.length = 0;
	if(write(vesa,&header,sizeof(header)) != sizeof(header)) {
		printe("Unable to send get-mode-request to vesa");
		return EXIT_FAILURE;
	}

	/* read response */
	sMsgDataVesaGetModeResp resp;
	if(read(vesa,&header,sizeof(header)) != sizeof(header) ||
			read(vesa,&resp,sizeof(resp)) != sizeof(resp)) {
		printe("Unable to read the get-mode-response from vesa");
		return EXIT_FAILURE;
	}

	/* store */
	screenWidth = resp.width;
	screenHeight = resp.height;
	colorDepth = resp.colorDepth;

	shmem = (u8*)joinSharedMem("vesa");
	if(shmem == NULL) {
		printe("Unable to join shared memory 'vesa'");
		return false;
	}

	return true;
}

tCoord win_getScreenWidth(void) {
	return screenWidth;
}

tCoord win_getScreenHeight(void) {
	return screenHeight;
}

void win_setCursor(tCoord x,tCoord y) {
	cursorMsg.data.x = x;
	cursorMsg.data.y = y;
	write(vesa,&cursorMsg,sizeof(cursorMsg));
}

tWinId win_create(sMsgDataWinCreateReq msg) {
	tWinId i;
	for(i = 0; i < WINDOW_COUNT; i++) {
		if(windows[i].id == WINID_UNSED) {
			windows[i].id = i;
			windows[i].x = msg.x;
			windows[i].y = msg.y;
			windows[i].z = i;
			windows[i].width = msg.width;
			windows[i].height = msg.height;
			windows[i].owner = msg.owner;
			windows[i].buffer = NULL;
			/* TODO what keymap to set? */
			windows[i].keymap = 1;
			win_setActive(i);
			return i;
		}
	}
	return WINID_UNSED;
}

bool win_exists(tWinId id) {
	return id < WINDOW_COUNT && windows[id].id != WINID_UNSED;
}

sWindow *win_getAt(tCoord x,tCoord y) {
	tWinId i;
	s16 maxz = -1;
	tWinId winId = WINDOW_COUNT;
	sWindow *w = windows;
	for(i = 0; i < WINDOW_COUNT; i++) {
		if(w->id != WINID_UNSED && w->z > maxz &&
				x >= w->x && x < w->x + w->width &&
				y >= w->y && y < w->y + w->height) {
			winId = i;
			maxz = w->z;
		}
		w++;
	}
	if(winId != WINDOW_COUNT)
		return windows + winId;
	return NULL;
}

sWindow *win_getActive(void) {
	if(activeWindow != WINDOW_COUNT)
		return windows + activeWindow;
	return NULL;
}

void win_setActive(tWinId id) {
	tWinId i;
	s16 curz = windows[id].z;
	s16 maxz = 0;
	sWindow *w = windows;
	for(i = 0; i < WINDOW_COUNT; i++) {
		if(w->id != WINID_UNSED && w->z > curz) {
			if(w->z > maxz)
				maxz = w->z;
			w->z--;
		}
		w++;
	}
	if(maxz > 0)
		windows[id].z = maxz;
	activeWindow = id;
}

void win_moveTo(tWinId window,tCoord x,tCoord y) {
	sRectangle *old = (sRectangle*)malloc(sizeof(sRectangle));
	sRectangle *new = (sRectangle*)malloc(sizeof(sRectangle));

	/* save old position */
	old->x = windows[window].x;
	old->y = windows[window].y;
	old->width = windows[window].width;
	old->height = windows[window].height;

	/* create rectangle for new position */
	new->x = windows[window].x = x;
	new->y = windows[window].y = y;
	new->width = old->width;
	new->height = old->height;

	/*debugf("old=%d,%d:%d,%d, ",old->x,old->y,old->width,old->height);
	debugf("new=%d,%d:%d,%d\n",new->x,new->y,new->width,new->height);*/

	u32 i,count;

	/* clear old position */
	sRectangle *rects = rectSplit(old,new,&count);
	if(count > 0) {
		/* if there is an intersection, use the splitted parts */
		for(i = 0; i < count; i++) {
			/* repaintWindow() will free one rect, but we've allocated multiple rects at once :/ */
			sRectangle *tmp = (sRectangle*)malloc(sizeof(sRectangle));
			memcpy(tmp,rects + i,sizeof(sRectangle));
			tmp->window = WINDOW_COUNT;
			win_repaint(tmp,NULL,-1);
		}
		free(rects);
		free(old);
	}
	else {
		/* no intersection, so use the whole old rectangle */
		old->window = WINDOW_COUNT;
		win_repaint(old,NULL,-1);
	}

	/* repaint new position */
	win_repaint(new,windows + window,windows[window].z);
}

static void win_repaint(sRectangle *r,sWindow *win,s16 z) {
	sRectangle *rect;
	tFD aWin;
	sSLNode *n;
	sSLList *list = sll_create();
	if(list == NULL) {
		printe("Unable to create list");
		exit(EXIT_FAILURE);
	}

	win_getRepaintRegions(list,0,win,z,r);
	for(n = sll_begin(list); n != NULL; n = n->next) {
		rect = (sRectangle*)n->data;
		/*debugf("-> Rect of %d=%d,%d:%d,%d",
				rect->window,rect->x,rect->y,rect->width,rect->height);*/

		/* ignore invalid values */
		/* FIXME where do they come from? */
		if(rect->x + rect->width > screenWidth || rect->y + rect->height > screenHeight ||
				rect->width > screenWidth || rect->height > screenHeight)
			continue;

		/* if it doesn't belong to a window, we have to clear it */
		if(rect->window == WINDOW_COUNT) {
			/*debugf("\n");*/
			win_clearRegion(shmem,rect->x,rect->y,rect->width,rect->height);
		}
		else {
			/*debugf(" (winpos=%d,%d)\n",windows[rect->window].x,windows[rect->window].y);*/
			/* send the window a repaint-request */
			repaintMsg.data.x = rect->x - windows[rect->window].x;
			repaintMsg.data.y = rect->y - windows[rect->window].y;
			repaintMsg.data.width = rect->width;
			repaintMsg.data.height = rect->height;
			repaintMsg.data.window = rect->window;
			aWin = getClientProc(servId,windows[rect->window].owner);
			if(aWin >= 0) {
				write(aWin,&repaintMsg,sizeof(repaintMsg));
				close(aWin);
			}
		}
	}
	sll_destroy(list,true);
}

static void win_getRepaintRegions(sSLList *list,tWinId id,sWindow *win,s16 z,sRectangle *r) {
	sRectangle *rects;
	sRectangle wr;
	sRectangle *inter;
	sWindow *w;
	u32 count;
	bool intersect;
	for(; id < WINDOW_COUNT; id++) {
		w = windows + id;
		/* skip unused, ourself and rects behind ourself */
		if((win && w->id == win->id) || w->id == WINID_UNSED || w->z < z)
			continue;

		/* build window-rect */
		wr.x = w->x;
		wr.y = w->y;
		wr.width = w->width;
		wr.height = w->height;
		/* split r by the rectangle */
		rects = rectSplit(r,&wr,&count);

		/* the window has to repaint the intersection, if there is any */
		inter = (sRectangle*)malloc(sizeof(sRectangle));
		if(inter == NULL) {
			printe("Unable to allocate mem");
			exit(EXIT_FAILURE);
		}
		intersect = rectIntersect(&wr,r,inter);
		if(intersect)
			win_getRepaintRegions(list,id + 1,w,w->z,inter);
		else
			free(inter);

		if(rects) {
			/* split all by all other windows */
			while(count-- > 0)
				win_getRepaintRegions(list,id + 1,win,z,rects + count);
		}

		/* if we made a recursive call we can leave here */
		if(rects || intersect) {
			/* forget old rect and stop here */
			free(r);
			return;
		}
	}

	/* no split, so its on top */
	if(win)
		r->window = win->id;
	else
		r->window = WINDOW_COUNT;
	sll_append(list,r);
}

static void win_clearRegion(u8 *mem,tCoord x,tCoord y,tSize width,tSize height) {
	tCoord ysave = y;
	tCoord maxy = y + height;
	u32 count = width * PIXEL_SIZE;
	mem += (y * screenWidth + x) * PIXEL_SIZE;
	while(y <= maxy) {
		memset(mem,0,count);
		mem += screenWidth * PIXEL_SIZE;
		y++;
	}

	win_notifyVesa(x,ysave,width,height);
}

static void win_notifyVesa(tCoord x,tCoord y,tSize width,tSize height) {
	vesaMsg.data.x = x;
	vesaMsg.data.y = y;
	vesaMsg.data.width = width;
	vesaMsg.data.height = height;
	write(vesa,&vesaMsg,sizeof(sMsgVesaUpdate));
}
