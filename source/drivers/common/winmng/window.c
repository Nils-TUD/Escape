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
#include <esc/driver/screen.h>
#include <esc/driver/uimng.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "window.h"
#include "listener.h"
#include "preview.h"

#define PIXEL_SIZE	(mode.bitsPerPixel / 8)
#define ABS(a)		((a) < 0 ? -(a) : (a))

static int win_createBuf(sWindow *win,gwinid_t id,gsize_t width,gsize_t height,const char *winmng);
static void win_destroyBuf(sWindow *win);
static gwinid_t win_getTop(void);
static bool win_validateRect(sRectangle *r);
static void win_repaint(sRectangle *r,sWindow *win,gpos_t z);
static void win_sendActive(gwinid_t id,bool isActive,gpos_t mouseX,gpos_t mouseY);
static void win_getRepaintRegions(sSLList *list,gwinid_t id,sWindow *win,gpos_t z,sRectangle *r);
static void win_clearRegion(uint8_t *mem,const sRectangle *r);
static void win_copyRegion(uint8_t *mem,const sRectangle *r,gwinid_t id);
static void win_notifyWinCreate(gwinid_t id,const char *title);
static void win_notifyWinActive(gwinid_t id);
static void win_notifyWinDestroy(gwinid_t id);

static int uimng;
static int drvId;

static sScreenMode mode;
static uint8_t *shmem;

static sMsg msg;

static size_t activeWindow = WINDOW_COUNT;
static size_t topWindow = WINDOW_COUNT;
static sWindow windows[WINDOW_COUNT];

int win_init(int sid,int uifd,gsize_t width,gsize_t height,gcoldepth_t bpp,const char *shmname) {
	drvId = sid;
	uimng = uifd;

	/* mark windows unused */
	for(gwinid_t i = 0; i < WINDOW_COUNT; i++)
		windows[i].id = WINID_UNUSED;

	/* find desired mode */
	if(screen_findGraphicsMode(uifd,width,height,bpp,&mode) < 0)
		error("Unable to find suitable mode");

	/* create shm */
	int res = screen_createShm(&mode,(char**)&shmem,shmname,VID_MODE_TYPE_GUI,0644);
	if(res < 0)
		error("Unable to create shm file '%s'",shmname);
	return mode.id;
}

const sScreenMode *win_getMode(void) {
	return &mode;
}

void win_setCursor(gpos_t x,gpos_t y,uint cursor) {
	msg.args.arg1 = x;
	msg.args.arg2 = y;
	msg.args.arg3 = cursor;
	send(uimng,MSG_SCR_SETCURSOR,&msg,sizeof(msg.args));
}

static int win_createBuf(sWindow *win,gwinid_t id,gsize_t width,gsize_t height,const char *winmng) {
	char name[32];
	snprintf(name,sizeof(name),"%s-win%d",winmng,id);
	win->shmfd = shm_open(name,IO_READ | IO_WRITE | IO_CREATE,0644);
	if(win->shmfd < 0)
		return win->shmfd;
	win->shmaddr = mmap(NULL,width * height * (mode.bitsPerPixel / 8),0,
			PROT_READ | PROT_WRITE,MAP_SHARED,win->shmfd,0);
	if(win->shmaddr == NULL) {
		close(win->shmfd);
		shm_unlink(name);
		return errno;
	}
	memclear(win->shmaddr,width * height * (mode.bitsPerPixel / 8));
	return 0;
}

static void win_destroyBuf(sWindow *win) {
	char name[16];
	snprintf(name,sizeof(name),"win-%d",win->id);
	close(win->shmfd);
	munmap(win->shmaddr);
	shm_unlink(name);
}

gwinid_t win_create(gpos_t x,gpos_t y,gsize_t width,gsize_t height,int owner,uint style,
		gsize_t titleBarHeight,const char *title,const char *winmng) {
	gwinid_t i;
	for(i = 0; i < WINDOW_COUNT; i++) {
		if(windows[i].id == WINID_UNUSED) {
			if(win_createBuf(windows + i,i,width,height,winmng) < 0)
				return WINID_UNUSED;

			windows[i].id = i;
			windows[i].x = x;
			windows[i].y = y;
			if(style == WIN_STYLE_DESKTOP)
				windows[i].z = 0;
			else {
				gwinid_t top = win_getTop();
				if(top != WINID_UNUSED)
					windows[i].z = windows[top].z + 1;
				else
					windows[i].z = 1;
			}
			windows[i].width = width;
			windows[i].height = height;
			windows[i].owner = owner;
			windows[i].style = style;
			windows[i].titleBarHeight = titleBarHeight;
			windows[i].ready = false;
			win_notifyWinCreate(i,title);
			return i;
		}
	}
	return WINID_UNUSED;
}

void win_destroyWinsOf(int cid,gpos_t mouseX,gpos_t mouseY) {
	gwinid_t id;
	for(id = 0; id < WINDOW_COUNT; id++) {
		if(windows[id].id != WINID_UNUSED && windows[id].owner == cid)
			win_destroy(id,mouseX,mouseY);
	}
}

void win_destroy(gwinid_t id,gpos_t mouseX,gpos_t mouseY) {
	sRectangle *old;
	/* destroy shm area */
	win_destroyBuf(windows + id);

	/* mark unused */
	windows[id].id = WINID_UNUSED;
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
	if(activeWindow == id || topWindow == id) {
		gwinid_t winId = win_getTop();
		if(winId != WINID_UNUSED)
			win_setActive(winId,false,mouseX,mouseY);
	}
}

sWindow *win_get(gwinid_t id) {
	if(id >= WINDOW_COUNT || windows[id].id == WINID_UNUSED)
		return NULL;
	return windows + id;
}

bool win_exists(gwinid_t id) {
	return id < WINDOW_COUNT && windows[id].id != WINID_UNUSED;
}

sWindow *win_getAt(gpos_t x,gpos_t y) {
	gwinid_t i;
	gpos_t maxz = -1;
	gwinid_t winId = WINDOW_COUNT;
	sWindow *w = windows;
	for(i = 0; i < WINDOW_COUNT; i++) {
		if(w->id != WINID_UNUSED && w->z > maxz &&
				x >= w->x && x < (gpos_t)(w->x + w->width) &&
				y >= w->y && y < (gpos_t)(w->y + w->height)) {
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
	if(id != WINDOW_COUNT && windows[id].style != WIN_STYLE_DESKTOP) {
		for(i = 0; i < WINDOW_COUNT; i++) {
			if(w->id != WINID_UNUSED && w->z > curz && w->style != WIN_STYLE_POPUP) {
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
		if(windows[activeWindow].style != WIN_STYLE_DESKTOP)
			topWindow = id;
		if(activeWindow != WINDOW_COUNT) {
			sRectangle *new;
			win_sendActive(activeWindow,true,mouseX,mouseY);
			win_notifyWinActive(activeWindow);

			if(repaint && windows[activeWindow].style != WIN_STYLE_DESKTOP) {
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
	preview_set(shmem,x,y,width,height,2);
}

void win_previewMove(gwinid_t window,gpos_t x,gpos_t y) {
	sWindow *w = windows + window;
	preview_set(shmem,x,y,w->width,w->height,2);
}

void win_resize(gwinid_t window,gpos_t x,gpos_t y,gsize_t width,gsize_t height,const char *winmng) {
	/* exchange buffer */
	win_destroyBuf(windows + window);
	assert(win_createBuf(windows + window,window,width,height,winmng) == 0);

	if(x != windows[window].x || y != windows[window].y)
		win_moveTo(window,x,y,width,height);
	else {
		gsize_t oldWidth = windows[window].width;
		gsize_t oldHeight = windows[window].height;
		windows[window].width = width;
		windows[window].height = height;

		/* remove preview */
		preview_set(shmem,0,0,0,0,0);

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
	preview_set(shmem,0,0,0,0,0);

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
	win->ready = true;

	sRectangle rect;
	sRectangle *r = topWindow == window ? &rect : (sRectangle*)malloc(sizeof(sRectangle));
	r->x = win->x + x;
	r->y = win->y + y;
	r->width = width;
	r->height = height;
	r->window = win->id;
	if(win_validateRect(r)) {
		if(topWindow == window)
			win_copyRegion(shmem,r,window);
		else
			win_repaint(r,win,win->z);
	}
}

static gwinid_t win_getTop(void) {
	gwinid_t i,winId = WINID_UNUSED;
	int maxz = -1;
	sWindow *w = windows;
	for(i = 0; i < WINDOW_COUNT; i++) {
		if(w->id != WINID_UNUSED && w->z > maxz) {
			winId = i;
			maxz = w->z;
		}
		w++;
	}
	return winId;
}

static bool win_validateRect(sRectangle *r) {
	if(r->x < 0) {
		if(-r->x > (gpos_t)r->width)
			return false;
		r->width += r->x;
		r->x = 0;
	}
	if(r->x >= (gpos_t)mode.width || r->y >= (gpos_t)mode.height)
		return false;
	r->width = MIN(mode.width - r->x,r->width);
	r->height = MIN(mode.height - r->y,r->height);
	return true;
}

static void win_repaint(sRectangle *r,sWindow *win,gpos_t z) {
	sRectangle *rect;
	sSLNode *n;
	sSLList list;
	sll_init(&list,malloc,free);

	win_getRepaintRegions(&list,0,win,z,r);
	for(n = sll_begin(&list); n != NULL; n = n->next) {
		rect = (sRectangle*)n->data;

		/* validate rect */
		if(!win_validateRect(rect))
			continue;

		/* if it doesn't belong to a window, we have to clear it */
		if(rect->window == WINDOW_COUNT)
			win_clearRegion(shmem,rect);
		/* otherwise copy from the window buffer */
		else
			win_copyRegion(shmem,rect,rect->window);
	}

	/* free list elements */
	sll_clear(&list,true);
}

static void win_sendActive(gwinid_t id,bool isActive,gpos_t mouseX,gpos_t mouseY) {
	msg.args.arg1 = id;
	msg.args.arg2 = isActive;
	msg.args.arg3 = mouseX;
	msg.args.arg4 = mouseY;
	if(send(windows[id].owner,MSG_WIN_SET_ACTIVE_EV,&msg,sizeof(msg.args)) < 0)
		printe("Unable to send active-event for window %u",id);
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
		if((win && w->id == win->id) || w->id == WINID_UNUSED || w->z < z || !w->ready)
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

static void win_clearRegion(uint8_t *mem,const sRectangle *r) {
	gpos_t y = r->y;
	size_t count = r->width * PIXEL_SIZE;
	gpos_t maxy = y + r->height;
	mem += (y * mode.width + r->x) * PIXEL_SIZE;
	while(y < maxy) {
		memclear(mem,count);
		mem += mode.width * PIXEL_SIZE;
		y++;
	}

	preview_updateRect(mem,r->x,r->y,r->width,r->height);
	win_notifyUimng(r->x,r->y,r->width,r->height);
}

static void win_copyRegion(uint8_t *mem,const sRectangle *r,gwinid_t id) {
	gpos_t x = r->x - windows[id].x;
	gpos_t y = r->y - windows[id].y;

	uint8_t *src,*dst;
	gpos_t endy = y + r->height;
	size_t count = r->width * PIXEL_SIZE;
	size_t srcAdd = windows[id].width * PIXEL_SIZE;
	size_t dstAdd = mode.width * PIXEL_SIZE;
	src = (uint8_t*)windows[id].shmaddr + (y * windows[id].width + x) * PIXEL_SIZE;
	dst = mem + ((windows[id].y + y) * mode.width + (windows[id].x + x)) * PIXEL_SIZE;

	while(y < endy) {
		memcpy(dst,src,count);
		src += srcAdd;
		dst += dstAdd;
		y++;
	}

	preview_updateRect(mem,windows[id].x + x,windows[id].y + (endy - r->height),r->width,r->height);
	win_notifyUimng(windows[id].x + x,windows[id].y + (endy - r->height),r->width,r->height);
}

void win_notifyUimng(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	if(x < 0) {
		width += x;
		x = 0;
	}
	screen_update(uimng,x,y,MIN(mode.width - x,width),MIN(mode.height - y,height));
}

static void win_notifyWinCreate(gwinid_t id,const char *title) {
	if(windows[id].style == WIN_STYLE_POPUP || windows[id].style == WIN_STYLE_DESKTOP)
		return;
	msg.str.arg1 = id;
	strcpy(msg.str.s1,title);
	listener_notify(MSG_WIN_CREATE_EV,&msg,sizeof(msg.str));
}

static void win_notifyWinActive(gwinid_t id) {
	if(windows[id].style == WIN_STYLE_POPUP)
		return;
	msg.args.arg1 = id;
	listener_notify(MSG_WIN_ACTIVE_EV,&msg,sizeof(msg.args));
}

static void win_notifyWinDestroy(gwinid_t id) {
	if(windows[id].style == WIN_STYLE_POPUP || windows[id].style == WIN_STYLE_DESKTOP)
		return;
	msg.args.arg1 = id;
	listener_notify(MSG_WIN_DESTROY_EV,&msg,sizeof(msg.args));
}


#if DEBUGGING

void win_dbg_print(void) {
	gwinid_t i;
	sWindow *w = windows;
	printf("Windows:\n");
	for(i = 0; i < WINDOW_COUNT; i++) {
		if(w->id != WINID_UNUSED) {
			printf("\t[%d] @(%d,%d), size=(%d,%d), z=%d, owner=%d, style=%d\n",
					i,w->x,w->y,w->width,w->height,w->z,w->owner,w->style);
		}
		w++;
	}
}

#endif
