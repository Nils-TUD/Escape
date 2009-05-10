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

#ifndef WINDOW_H_
#define WINDOW_H_

#include <esc/common.h>
#include <esc/messages.h>

#define WINDOW_COUNT					32
#define WINID_UNSED						WINDOW_COUNT

#define PIXEL_SIZE						(colorDepth / 8)

#define WIN_STYLE_DEFAULT				0
#define WIN_STYLE_POPUP					1

typedef u16 tSize;
typedef u16 tCoord;
typedef u32 tColor;
typedef u16 tWinId;

/* a window */
typedef struct {
	s16 x;
	s16 y;
	s16 z;
	tSize width;
	tSize height;
	tWinId id;
	tPid owner;
	u8 style;
	u8 keymap;
} sWindow;

/**
 * Inits all window-stuff
 *
 * @param sid the service-id
 * @return true on success
 */
bool win_init(tServ sid);

/**
 * @return the screen-width
 */
tCoord win_getScreenWidth(void);

/**
 * @return the screen-height
 */
tCoord win_getScreenHeight(void);

/**
 * Sets the cursor at given position (writes to vesa)
 *
 * @param x the x-coordinate
 * @param y the y-coordinate
 */
void win_setCursor(tCoord x,tCoord y);

/**
 * Creates a new window from given create-message
 *
 * @param msg the create-message
 * @return the window-id or WINID_UNUSED if no slot is free
 */
tWinId win_create(sMsgDataWinCreateReq msg);

/**
 * Destroys all windows of the given process
 *
 * @param pid the process-id
 * @param mouseX the current x-coordinate of the mouse
 * @param mouseY the current y-coordinate of the mouse
 */
void win_destroyWinsOf(tPid pid,tCoord mouseX,tCoord mouseY);

/**
 * Destroys the given window
 *
 * @param id the window-id
 * @param mouseX the current x-coordinate of the mouse
 * @param mouseY the current y-coordinate of the mouse
 */
void win_destroy(tWinId id,tCoord mouseX,tCoord mouseY);

/**
 * @param id the window-id
 * @return wether the window with given id exists
 */
bool win_exists(tWinId id);

/**
 * Determines the window at given position
 *
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @return the window or NULL
 */
sWindow *win_getAt(tCoord x,tCoord y);

/**
 * @return the currently active window; NULL if no one is active
 */
sWindow *win_getActive(void);

/**
 * Sets the active window to the given one. This will put <id> to the front and windows that
 * are above one step behind (z-coordinate)
 *
 * @param id the window-id
 * @param repaint wether the window should receive a repaint-event
 * @param mouseX the current x-coordinate of the mouse
 * @param mouseY the current y-coordinate of the mouse
 */
void win_setActive(tWinId id,bool repaint,tCoord mouseX,tCoord mouseY);

/**
 * Moves the given window to given position
 *
 * @param window the window-id
 * @param x the x-coordinate
 * @param y the y-coordinate
 */
void win_moveTo(tWinId window,tCoord x,tCoord y);

#endif /* WINDOW_H_ */
