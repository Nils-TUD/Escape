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
#include <esc/rect.h>

#define WINDOW_COUNT					32
#define WINID_UNSED						WINDOW_COUNT

#define PIXEL_SIZE						(vesaInfo.bitsPerPixel / 8)

#define WIN_STYLE_DEFAULT				0
#define WIN_STYLE_POPUP					1
#define WIN_STYLE_DESKTOP				2

typedef ushort tSize;
typedef short tCoord;
typedef uint32_t tColor;
typedef ushort tWinId;

/* a window */
typedef struct {
	tCoord x;
	tCoord y;
	tCoord z;
	tSize width;
	tSize height;
	tWinId id;
	tid_t owner;
	uint style;
} sWindow;

/**
 * Inits all window-stuff
 *
 * @param sid the driver-id
 * @return true on success
 */
bool win_init(int sid);

/**
 * @return whether the window-manager is enabled
 */
bool win_isEnabled(void);

/**
 * Enables or disables the window-manager
 *
 * @param en the new value
 */
void win_setEnabled(bool en);

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
 * @param cursor the cursor to use
 */
void win_setCursor(tCoord x,tCoord y,uint cursor);

/**
 * Creates a new window from given create-message
 *
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @param width the width
 * @param height the height
 * @param owner the owner-id
 * @param style style-attributes
 * @return the window-id or WINID_UNUSED if no slot is free
 */
tWinId win_create(tCoord x,tCoord y,tSize width,tSize height,inode_t owner,uint style);

/**
 * Updates the whole screen
 */
void win_updateScreen(void);

/**
 * Destroys all windows of the given thread
 *
 * @param cid the client-id
 * @param mouseX the current x-coordinate of the mouse
 * @param mouseY the current y-coordinate of the mouse
 */
void win_destroyWinsOf(inode_t cid,tCoord mouseX,tCoord mouseY);

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
 * @return the window with given id or NULL
 */
sWindow *win_get(tWinId id);

/**
 * @param id the window-id
 * @return whether the window with given id exists
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
 * @param repaint whether the window should receive a repaint-event
 * @param mouseX the current x-coordinate of the mouse
 * @param mouseY the current y-coordinate of the mouse
 */
void win_setActive(tWinId id,bool repaint,tCoord mouseX,tCoord mouseY);

/**
 * Shows a preview for the given resize-operation
 *
 * @param window the window-id
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @param width the new width
 * @param height the new height
 */
void win_previewResize(tWinId window,tCoord x,tCoord y,tSize width,tSize height);

/**
 * Shows a preview for the given move-operation
 *
 * @param window the window-id
 * @param x the x-coordinate
 * @param y the y-coordinate
 */
void win_previewMove(tWinId window,tCoord x,tCoord y);

/**
 * Resizes the window to the given size
 *
 * @param window the window-id
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @param width the new width
 * @param height the new height
 */
void win_resize(tWinId window,tCoord x,tCoord y,tSize width,tSize height);

/**
 * Moves the given window to given position and optionally changes the size
 *
 * @param window the window-id
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @param width the new width
 * @param height the new height
 */
void win_moveTo(tWinId window,tCoord x,tCoord y,tSize width,tSize height);

/**
 * Sends update-events to the given window to update the given area
 *
 * @param window the window-id
 * @param x the x-coordinate in the window
 * @param y the y-coordinate in the window
 * @param width the width
 * @param height the height
 */
void win_update(tWinId window,tCoord x,tCoord y,tSize width,tSize height);

#if DEBUGGING

void win_dbg_print(void);

#endif

#endif /* WINDOW_H_ */
