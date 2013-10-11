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

#pragma once

#include <esc/common.h>
#include <esc/messages.h>
#include <esc/rect.h>

#define WINDOW_COUNT					32
#define WINID_UNUSED					WINDOW_COUNT

#define WIN_STYLE_DEFAULT				0
#define WIN_STYLE_POPUP					1
#define WIN_STYLE_DESKTOP				2

typedef uint32_t tColor;

/* a window */
typedef struct {
	gpos_t x;
	gpos_t y;
	gpos_t z;
	gsize_t width;
	gsize_t height;
	gwinid_t id;
	int owner;
	uint style;
	gsize_t titleBarHeight;
	int shmfd;
	void *shmaddr;
	bool ready;
} sWindow;

/**
 * Inits all window-stuff
 *
 * @param sid the device-id
 * @param uifd the uimanager fd
 * @param width the desired screen width
 * @param height the desired screen height
 * @param shmname the shared memory name
 * @return the mode id on success
 */
int win_init(int sid,int uifd,gsize_t width,gsize_t height,const char *shmname);

/**
 * @return the current mode
 */
const sScreenMode *win_getMode(void);

/**
 * Sets the cursor at given position (writes to vesa)
 *
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @param cursor the cursor to use
 */
void win_setCursor(gpos_t x,gpos_t y,uint cursor);

/**
 * Creates a new window from given create-message
 *
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @param width the width
 * @param height the height
 * @param owner the client-fd
 * @param style style-attributes
 * @param titleBarHeight the height of the titlebar of that window
 * @param title the title of the window
 * @return the window-id or WINID_UNUSED if no slot is free
 */
gwinid_t win_create(gpos_t x,gpos_t y,gsize_t width,gsize_t height,int owner,uint style,
	gsize_t titleBarHeight,const char *title);

/**
 * Destroys all windows of the given thread
 *
 * @param cid the client-fd
 * @param mouseX the current x-coordinate of the mouse
 * @param mouseY the current y-coordinate of the mouse
 */
void win_destroyWinsOf(int cid,gpos_t mouseX,gpos_t mouseY);

/**
 * Destroys the given window
 *
 * @param id the window-id
 * @param mouseX the current x-coordinate of the mouse
 * @param mouseY the current y-coordinate of the mouse
 */
void win_destroy(gwinid_t id,gpos_t mouseX,gpos_t mouseY);

/**
 * @param id the window-id
 * @return the window with given id or NULL
 */
sWindow *win_get(gwinid_t id);

/**
 * @param id the window-id
 * @return whether the window with given id exists
 */
bool win_exists(gwinid_t id);

/**
 * Determines the window at given position
 *
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @return the window or NULL
 */
sWindow *win_getAt(gpos_t x,gpos_t y);

/**
 * @return the currently active window; NULL if no one is active
 */
sWindow *win_getActive(void);

/**
 * Sets the active window to the given one. This will put <id> to the front and windows that
 * are above one step behind (z-coordinate)
 *
 * @param id the window-id
 * @param repaint whether the window should receive a repaint-event
 * @param mouseX the current x-coordinate of the mouse
 * @param mouseY the current y-coordinate of the mouse
 */
void win_setActive(gwinid_t id,bool repaint,gpos_t mouseX,gpos_t mouseY);

/**
 * Shows a preview for the given resize-operation
 *
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @param width the new width
 * @param height the new height
 */
void win_previewResize(gpos_t x,gpos_t y,gsize_t width,gsize_t height);

/**
 * Shows a preview for the given move-operation
 *
 * @param window the window-id
 * @param x the x-coordinate
 * @param y the y-coordinate
 */
void win_previewMove(gwinid_t window,gpos_t x,gpos_t y);

/**
 * Resizes the window to the given size
 *
 * @param window the window-id
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @param width the new width
 * @param height the new height
 */
void win_resize(gwinid_t window,gpos_t x,gpos_t y,gsize_t width,gsize_t height);

/**
 * Moves the given window to given position and optionally changes the size
 *
 * @param window the window-id
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @param width the new width
 * @param height the new height
 */
void win_moveTo(gwinid_t window,gpos_t x,gpos_t y,gsize_t width,gsize_t height);

/**
 * Sends update-events to the given window to update the given area
 *
 * @param window the window-id
 * @param x the x-coordinate in the window
 * @param y the y-coordinate in the window
 * @param width the width
 * @param height the height
 */
void win_update(gwinid_t window,gpos_t x,gpos_t y,gsize_t width,gsize_t height);

/**
 * Notifies the UI-manager that the given rectangle has changed.
 *
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @param width the width
 * @param height the height
 */
void win_notifyUimng(gpos_t x,gpos_t y,gsize_t width,gsize_t height);

#if DEBUGGING

void win_dbg_print(void);

#endif
