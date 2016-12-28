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

#pragma once

#include <esc/proto/ui.h>
#include <gui/graphics/rectangle.h>
#include <sys/common.h>
#include <sys/messages.h>

#include "input.h"
#include "preview.h"
#include "window.h"

class WinList {
	explicit WinList(int sid,esc::UI *ui,gsize_t width,gsize_t height,gcoldepth_t bpp);

public:
	/**
	 * Inits all window-stuff
	 *
	 * @param sid the device-id
	 * @param ui the ui
	 * @param width the desired screen width
	 * @param height the desired screen height
	 * @param bpp the desired bits per pixel
	 * @return the mode id on success
	 */
	static void create(int sid,esc::UI *ui,gsize_t width,gsize_t height,gcoldepth_t bpp) {
		_inst = new WinList(sid,ui,width,height,bpp);
	}

	/**
	 * @return the instance
	 */
	static WinList &get() {
		return *_inst;
	}

	/**
	 * @return the current theme
	 */
	const std::string &getTheme() const {
		return theme;
	}

	/**
	 * Sets the theme.
	 *
	 * @param name the new name
	 */
	void setTheme(const char *name);

	/**
	 * @return the current mode
	 */
	const esc::Screen::Mode &getMode() const {
		return mode;
	}

	/**
	 * @return the frame buffer for the whole screen
	 */
	esc::FrameBuffer *getFrameBuffer() {
		return fb;
	}

	/**
	 * Changes the mode to the given one.
	 *
	 * @param width the desired screen width
	 * @param height the desired screen height
	 * @param bpp the desired bits per pixel
	 * @return 0 on success
	 */
	int setMode(gsize_t width,gsize_t height,gcoldepth_t bpp);

	/**
	 * Sets the cursor at given position (writes to vesa)
	 *
	 * @param x the x-coordinate
	 * @param y the y-coordinate
	 * @param cursor the cursor to use
	 */
	void setCursor(gpos_t x,gpos_t y,uint cursor) {
		ui->setCursor(x,y,cursor);
	}

	/**
	 * Detaches the given fd from the windows that uses it.
	 *
	 * @param fd the fd for the event-channel
	 */
	void detachAll(int fd);

	/**
	 * Destroys all windows of the given thread
	 *
	 * @param cid the client-fd
	 */
	void destroyWinsOf(int cid);

	/**
	 * Creates a new window
	 *
	 * @param x the x-coordinate
	 * @param y the y-coordinate
	 * @param width the width
	 * @param height the height
	 * @param owner the window-owner (fd)
	 * @param style style-attributes
	 * @param titleBarHeight the height of the titlebar of that window
	 * @param title the title of the window
	 * @return the window id or an error code
	 */
	gwinid_t add(gpos_t x,gpos_t y,gsize_t width,gsize_t height,int owner,uint style,
		gsize_t titleBarHeight,const char *title);

	/**
	 * Removes the given window
	 *
	 * @param win the window
	 */
	void remove(Window *win);

	/**
	 * @param id the window-id
	 * @return the window with given id or NULL
	 */
	Window *get(gwinid_t id) {
		if(id >= WINDOW_COUNT || !windows[id])
			return NULL;
		return windows[id];
	}

	/**
	 * @param id the window-id
	 * @return whether the window with given id exists
	 */
	bool exists(gwinid_t id) {
		return id < WINDOW_COUNT && windows[id];
	}

	/**
	 * Determines the window at given position
	 *
	 * @param x the x-coordinate
	 * @param y the y-coordinate
	 * @return the window or NULL
	 */
	Window *getAt(gpos_t x,gpos_t y);

	/**
	 * Get window at the top
	 *
	 * @return the window
	 */
	Window *getTop();

	/**
	 * @return the currently active window; NULL if no one is active
	 */
	Window *getActive() {
		if(activeWindow != WINID_UNUSED)
			return windows[activeWindow];
		return NULL;
	}

	/**
	 * Makes the given window the active window. This will put it to the front and windows that
	 * are above one step behind (z-coordinate)
	 *
	 * @param win the window
	 * @param repaint whether the window should receive a repaint-event
	 * @param mouseX the current x-coordinate of the mouse
	 * @param mouseY the current y-coordinate of the mouse
	 * @param updateWinStack whether to update the window stack
	 */
	void setActive(Window *win,bool repaint,bool updateWinStack) {
		setActive(win,repaint,Input::get().getMouseX(),Input::get().getMouseY(),updateWinStack);
	}
	void setActive(Window *win,bool repaint,gpos_t mouseX,gpos_t mouseY,bool updateWinStack);

	/**
	 * Sends update-events to the window to update the given area
	 *
	 * @param win the window
	 * @param x the x-coordinate in the window
	 * @param y the y-coordinate in the window
	 * @param width the width
	 * @param height the height
	 */
	void update(Window *win,gpos_t x,gpos_t y,gsize_t width,gsize_t height);

	/**
	 * Repaints the given rectangle of the screen.
	 *
	 * @param r the rectangle
	 * @param win the window (NULL if the remaining area should be cleared)
	 * @param z the z-position (all windows behind are not painted)
	 */
	void repaint(const gui::Rectangle &r,Window *win,gpos_t z);

	/**
	 * Removes the preview rectangle, if necessary.
	 */
	void removePreview() {
		Preview::get().set(fb->addr(),0,0,0,0,0);
	}

	/**
	 * Shows a preview for the given resize-operation
	 *
	 * @param x the x-coordinate
	 * @param y the y-coordinate
	 * @param width the new width
	 * @param height the new height
	 */
	void previewResize(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		Preview::get().set(fb->addr(),x,y,width,height,2);
	}

	/**
	 * Shows a preview for the given move-operation
	 *
	 * @param x the x-coordinate
	 * @param y the y-coordinate
	 */
	void previewMove(Window *win,gpos_t x,gpos_t y) {
		Preview::get().set(fb->addr(),x,y,win->width(),win->height(),2);
	}

	/**
	 * Notifies the UI-manager that the given rectangle has changed.
	 *
	 * @param x the x-coordinate
	 * @param y the y-coordinate
	 * @param width the width
	 * @param height the height
	 */
	void notifyUimng(gpos_t x,gpos_t y,gsize_t width,gsize_t height);

private:
	void resetAll();
	bool validateRect(gui::Rectangle &r);
	void getRepaintRegions(std::vector<WinRect> &list,gwinid_t id,Window *win,gpos_t z,
		const gui::Rectangle &r);
	void clearRegion(char *mem,const gui::Rectangle &r);
	void copyRegion(char *mem,const gui::Rectangle &r,gwinid_t id);

	esc::UI *ui;
	int drvId;

	std::string theme;
	esc::Screen::Mode mode;
	esc::FrameBuffer *fb;

	size_t activeWindow;
	size_t topWindow;
	Window *windows[WINDOW_COUNT];
	static WinList *_inst;
};
