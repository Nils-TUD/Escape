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
#include <esc/rect.h>
#include <esc/mem.h>
#include <esc/proc.h>
#include <esc/driver.h>
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/sllist.h>
#include <esc/driver/vesa.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "window.h"
#include "listener.h"

static gwinid_t win_getTop(void);
static void win_repaint(sRectangle *r,sWindow *win,gpos_t z);
static void win_sendActive(gwinid_t id,bool isActive,gpos_t mouseX,gpos_t mouseY);
static void win_sendRepaint(gpos_t x,gpos_t y,gsize_t width,gsize_t height,gwinid_t id);
static void win_getRepaintRegions(sSLList *list,gwinid_t id,sWindow *win,gpos_t z,sRectangle *r);
static void win_clearRegion(uint8_t *mem,gpos_t x,gpos_t y,gsize_t width,gsize_t height);
static void win_notifyVesa(gpos_t x,gpos_t y,gsize_t width,gsize_t height);
static void win_notifyWinCreate(gwinid_t id,const char *title);
static void win_notifyWinActive(gwinid_t id);
static void win_notifyWinDestroy(gwinid_t id);

static int vesa;
static int drvId;
static sVESAInfo vesaInfo;

static bool enabled = false;
static sMsg msg;
static uint8_t *shmem;
static size_t activeWindow = WINDOW_COUNT;
static sWindow windows[WINDOW_COUNT];

bool win_init(int sid) {
	msgid_t mid;
	gwinid_t i;
	int fd;

	drvId = sid;

	/* mark windows unused */
	for(i = 0; i < WINDOW_COUNT; i++)
		windows[i].id = WINID_UNSED;

	vesa = open("/dev/vesa",IO_WRITE | IO_MSGS);
	if(vesa < 0)
		error("Unable to open /dev/vesa");

	/* request screen infos from vesa */
	if(send(vesa,MSG_VESA_GETMODE,&msg,sizeof(msg.args)) < 0)
		error("Unable to send get-mode-request to vesa");
	if(IGNSIGS(receive(vesa,&mid,&msg,sizeof(msg))) < 0 ||
			mid != MSG_VESA_GETMODE_RESP || msg.data.arg1 != 0)
		error("Unable to read the get-mode-response from vesa");

	/* store */
	memcpy(&vesaInfo,msg.data.d,sizeof(sVESAInfo));

	fd = shm_open(VESA_SHM_NAME,IO_READ | IO_WRITE,VESA_SHM_PERM);
	if(fd < 0)
		error("Unable to open shm file '%s'",VESA_SHM_NAME);
	size_t screenSize = vesaInfo.width * vesaInfo.height * (vesaInfo.bitsPerPixel / 8);
	shmem = (uint8_t*)mmap(NULL,screenSize,0,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
	if(shmem == NULL)
		error("Unable to join shared memory 'vesa'");
	return true;
}

bool win_isEnabled(void) {
	return enabled;
}

void win_setEnabled(bool en) {
	enabled = en;
	if(send(vesa,en ? MSG_VESA_ENABLE : MSG_VESA_DISABLE,NULL,0))
		error("Unable to enable/disable vesa");
}

gpos_t win_getScreenWidth(void) {
	return vesaInfo.width;
}

gpos_t win_getScreenHeight(void) {
	return vesaInfo.height;
}

void win_setCursor(gpos_t x,gpos_t y,uint cursor) {
	msg.args.arg1 = x;
	msg.args.arg2 = y;
	msg.args.arg3 = cursor;
	send(vesa,MSG_VESA_CURSOR,&msg,sizeof(msg.args));
}

gwinid_t win_create(gpos_t x,gpos_t y,gsize_t width,gsize_t height,inode_t owner,uint style,
		gsize_t titleBarHeight,const char *title) {
	gwinid_t i;
	for(i = 0; i < WINDOW_COUNT; i++) {
		if(windows[i].id == WINID_UNSED) {
			windows[i].id = i;
			windows[i].x = x;
			windows[i].y = y;
			if(style == WIN_STYLE_DESKTOP)
				windows[i].z = 0;
			else {
				gwinid_t top = win_getTop();
				if(top != WINID_UNSED)
					windows[i].z = windows[top].z + 1;
				else
					windows[i].z = 1;
			}
			windows[i].width = width;
			windows[i].height = height;
			windows[i].owner = owner;
			windows[i].style = style;
			windows[i].titleBarHeight = titleBarHeight;
			if(style != WIN_STYLE_POPUP && style != WIN_STYLE_DESKTOP)
				win_notifyWinCreate(i,title);
			return i;
		}
	}
	return WINID_UNSED;
}

void win_updateScreen(void) {
	send(vesa,MSG_VESA_SETMODE,NULL,0);
	win_notifyVesa(0,0,vesaInfo.width,vesaInfo.height);
}

void win_destroyWinsOf(inode_t cid,gpos_t mouseX,gpos_t mouseY) {
	gwinid_t id;
	for(id = 0; id < WINDOW_COUNT; id++) {
		if(windows[id].id != WINID_UNSED && windows[id].owner == cid)
			win_destroy(id,mouseX,mouseY);
	}
}

void win_destroy(gwinid_t id,gpos_t mouseX,gpos_t mouseY) {
	sRectangle *old;
	/* mark unused */
	windows[id].id = WINID_UNSED;
	if(windows[id].style != WIN_STYLE_POPUP && windows[id].style != WIN_STYLE_DESKTOP)
		win_notifyWinDestroy(id);

	/* repaint window-area */
	old = (sRectangle*)malloc(sizeof(sRectangle));
	old->x = windows[id].x;
	old->y = windows[id].y;
	old->width = windows[id].width;
	old->height = windows[id].height;
	old->window = WINDOW_COUNT;
	win_repaint(old,NULL,-1);

	/* set highest window active */
	if(activeWindow == id) {
		gwinid_t winId = win_getTop();
		if(winId != WINID_UNSED)
			win_setActive(winId,false,mouseX,mouseY);
	}
}

sWindow *win_get(gwinid_t id) {
	if(id >= WINDOW_COUNT || windows[id].id == WINID_UNSED)
		return NULL;
	return windows + id;
}

bool win_exists(gwinid_t id) {
	return id < WINDOW_COUNT && windows[id].id != WINID_UNSED;
}

sWindow *win_getAt(gpos_t x,gpos_t y) {
	gwinid_t i;
	gpos_t maxz = -1;
	gwinid_t winId = WINDOW_COUNT;
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

void win_setActive(gwinid_t id,bool repaint,gpos_t mouseX,gpos_t mouseY) {
	gwinid_t i;
	gpos_t curz = windows[id].z;
	gpos_t maxz = 0;
	sWindow *w = windows;
	if(id != WINDOW_COUNT) {
		for(i = 0; i < WINDOW_COUNT; i++) {
			if(w->id != WINID_UNSED && w->z > curz && w->style != WIN_STYLE_POPUP) {
				if(w->z > maxz)
					maxz = w->z;
				w->z--;
			}
			w++;
		}
		if(maxz > 0)
			windows[id].z = maxz;
	}

	if(id != activeWindow) {
		if(activeWindow != WINDOW_COUNT)
			win_sendActive(activeWindow,false,mouseX,mouseY);

		activeWindow = id;
		if(activeWindow != WINDOW_COUNT) {
			sRectangle *new;
			win_sendActive(activeWindow,true,mouseX,mouseY);
			if(windows[activeWindow].style != WIN_STYLE_POPUP)
				win_notifyWinActive(activeWindow);

			if(repaint) {
				new = (sRectangle*)malloc(sizeof(sRectangle));
				new->x = windows[activeWindow].x;
				new->y = windows[activeWindow].y;
				new->width = windows[activeWindow].width;
				new->height = windows[activeWindow].height;
				new->window = WINDOW_COUNT;
				win_repaint(new,windows + activeWindow,windows[activeWindow].z);
			}
		}
	}
}

void win_previewResize(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	msg.args.arg1 = x;
	msg.args.arg2 = y;
	msg.args.arg3 = width;
	msg.args.arg4 = height;
	msg.args.arg5 = 2;
	send(vesa,MSG_VESA_PREVIEWRECT,&msg,sizeof(msg.args));
}

void win_previewMove(gwinid_t window,gpos_t x,gpos_t y) {
	sWindow *w = windows + window;
	msg.args.arg1 = x;
	msg.args.arg2 = y;
	msg.args.arg3 = w->width;
	msg.args.arg4 = w->height;
	msg.args.arg5 = 2;
	send(vesa,MSG_VESA_PREVIEWRECT,&msg,sizeof(msg.args));
}

void win_resize(gwinid_t window,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	if(x != windows[window].x || y != windows[window].y)
		win_moveTo(window,x,y,width,height);
	else {
		gsize_t oldWidth = windows[window].width;
		gsize_t oldHeight = windows[window].height;
		windows[window].width = width;
		windows[window].height = height;

		/* remove preview */
		msg.args.arg5 = 0;
		send(vesa,MSG_VESA_PREVIEWRECT,&msg,sizeof(msg.args));

		if(width < oldWidth) {
			sRectangle *r = (sRectangle*)malloc(sizeof(sRectangle));
			r->x = windows[window].x + width;
			r->y = windows[window].y;
			r->width = oldWidth - width;
			r->height = oldHeight;
			r->window = WINDOW_COUNT;
			win_repaint(r,NULL,-1);
		}
		if(height < oldHeight) {
			sRectangle *r = (sRectangle*)malloc(sizeof(sRectangle));
			r->x = windows[window].x;
			r->y = windows[window].y + height;
			r->width = oldWidth;
			r->height = oldHeight - height;
			r->window = WINDOW_COUNT;
			win_repaint(r,NULL,-1);
		}
	}
}

void win_moveTo(gwinid_t window,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	size_t i,count;
	sRectangle **rects;
	sRectangle *old = (sRectangle*)malloc(sizeof(sRectangle));
	sRectangle *new = (sRectangle*)malloc(sizeof(sRectangle));

	/* remove preview */
	msg.args.arg5 = 0;
	send(vesa,MSG_VESA_PREVIEWRECT,&msg,sizeof(msg.args));

	/* save old position */
	old->x = windows[window].x;
	old->y = windows[window].y;
	old->width = windows[window].width;
	old->height = windows[window].height;

	/* create rectangle for new position */
	new->x = windows[window].x = x;
	new->y = windows[window].y = y;
	new->width = windows[window].width = width;
	new->height = windows[window].height = height;

	/* clear old position */
	rects = rectSubstract(old,new,&count);
	if(count > 0) {
		/* if there is an intersection, use the splitted parts */
		for(i = 0; i < count; i++) {
			rects[i]->window = WINDOW_COUNT;
			win_repaint(rects[i],NULL,-1);
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

void win_update(gwinid_t window,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	sWindow *win = windows + window;
	sRectangle *r = (sRectangle*)malloc(sizeof(sRectangle));
	r->x = win->x + x;
	r->y = win->y + y;
	r->width = width;
	r->height = height;
	r->window = win->id;
	win_repaint(r,win,win->z);
}

static gwinid_t win_getTop(void) {
	gwinid_t i,winId = WINID_UNSED;
	int maxz = -1;
	sWindow *w = windows;
	for(i = 0; i < WINDOW_COUNT; i++) {
		if(w->id != WINID_UNSED && w->z > maxz) {
			winId = i;
			maxz = w->z;
		}
		w++;
	}
	return winId;
}

static void win_repaint(sRectangle *r,sWindow *win,gpos_t z) {
	sRectangle *rect;
	sSLNode *n;
	sSLList *list = sll_create();
	if(list == NULL) {
		printe("Unable to create list");
		exit(EXIT_FAILURE);
	}

	win_getRepaintRegions(list,0,win,z,r);
	for(n = sll_begin(list); n != NULL; n = n->next) {
		rect = (sRectangle*)n->data;

		/* if it doesn't belong to a window, we have to clear it */
		if(rect->window == WINDOW_COUNT)
			win_clearRegion(shmem,rect->x,rect->y,rect->width,rect->height);
		/* send the window a repaint-request */
		else
			win_sendRepaint(rect->x,rect->y,rect->width,rect->height,rect->window);
	}

	/* free data, too */
	sll_destroy(list,true);
}

static void win_sendActive(gwinid_t id,bool isActive,gpos_t mouseX,gpos_t mouseY) {
	int aWin = getclient(drvId,windows[id].owner);
	if(aWin >= 0) {
		msg.args.arg1 = id;
		msg.args.arg2 = isActive;
		msg.args.arg3 = mouseX;
		msg.args.arg4 = mouseY;
		send(aWin,MSG_WIN_SET_ACTIVE_EV,&msg,sizeof(msg.args));
		close(aWin);
	}
}

static void win_sendRepaint(gpos_t x,gpos_t y,gsize_t width,gsize_t height,gwinid_t id) {
	int aWin = getclient(drvId,windows[id].owner);
	if(aWin >= 0) {
		if(x - windows[id].x < 0) {
			width += x - windows[id].x;
			x = -windows[id].x;
		}
		msg.args.arg1 = x - windows[id].x;
		msg.args.arg2 = y - windows[id].y;
		msg.args.arg3 = width;
		msg.args.arg4 = height;
		msg.args.arg5 = id;
		send(aWin,MSG_WIN_UPDATE_EV,&msg,sizeof(msg.args));
		close(aWin);
	}
}

static void win_getRepaintRegions(sSLList *list,gwinid_t id,sWindow *win,gpos_t z,sRectangle *r) {
	sRectangle **rects;
	sRectangle wr;
	sRectangle *inter;
	sWindow *w;
	size_t count;
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
		rects = rectSubstract(r,&wr,&count);

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
				win_getRepaintRegions(list,id + 1,win,z,rects[count]);
			free(rects);
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

static void win_clearRegion(uint8_t *mem,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	gpos_t ysave = y;
	gpos_t maxy;
	size_t count;
	if(x < 0) {
		if(-x > width)
			return;
		width += x;
		x = 0;
	}
	if(x >= vesaInfo.width || y >= vesaInfo.height)
		return;
	width = MIN(vesaInfo.width - x,width);
	count = width * PIXEL_SIZE;
	maxy = MIN(vesaInfo.height,y + height);
	mem += (y * vesaInfo.width + x) * PIXEL_SIZE;
	while(y < maxy) {
		memclear(mem,count);
		mem += vesaInfo.width * PIXEL_SIZE;
		y++;
	}

	win_notifyVesa(x,ysave,width,height);
}

static void win_notifyVesa(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	if(x < 0) {
		width += x;
		x = 0;
	}
	msg.args.arg1 = x;
	msg.args.arg2 = y;
	msg.args.arg3 = MIN(vesaInfo.width - x,width);
	msg.args.arg4 = MIN(vesaInfo.height - y,height);
	send(vesa,MSG_VESA_UPDATE,&msg,sizeof(msg.args));
}

static void win_notifyWinCreate(gwinid_t id,const char *title) {
	msg.str.arg1 = id;
	strcpy(msg.str.s1,title);
	listener_notify(MSG_WIN_CREATE_EV,&msg,sizeof(msg.str));
}

static void win_notifyWinActive(gwinid_t id) {
	msg.args.arg1 = id;
	listener_notify(MSG_WIN_ACTIVE_EV,&msg,sizeof(msg.args));
}

static void win_notifyWinDestroy(gwinid_t id) {
	msg.args.arg1 = id;
	listener_notify(MSG_WIN_DESTROY_EV,&msg,sizeof(msg.args));
}


#if DEBUGGING

void win_dbg_print(void) {
	gwinid_t i;
	sWindow *w = windows;
	printf("Windows:\n");
	for(i = 0; i < WINDOW_COUNT; i++) {
		if(w->id != WINID_UNSED) {
			printf("\t[%d] @(%d,%d), size=(%d,%d), z=%d, owner=%d, style=%d\n",
					i,w->x,w->y,w->width,w->height,w->z,w->owner,w->style);
		}
		w++;
	}
}

#endif
