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

#include <esc/col/dlisttreap.h>
#include <esc/proto/ui.h>
#include <gui/graphics/rectangle.h>
#include <sys/common.h>
#include <sys/messages.h>
#include <mutex>

#include "input.h"
#include "preview.h"
#include "window.h"

class WinList {
	friend class Window;

	explicit WinList(int sid,esc::UI *ui,const gui::Size &size,gcoldepth_t bpp);

public:
	/**
	 * Inits all window-stuff
	 *
	 * @param sid the device-id
	 * @param ui the ui
	 * @param size the desired screen size
	 * @param bpp the desired bits per pixel
	 * @return the mode id on success
	 */
	static void create(int sid,esc::UI *ui,const gui::Size &size,gcoldepth_t bpp) {
		_inst = new WinList(sid,ui,size,bpp);
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
	 * @param size the desired screen size
	 * @param bpp the desired bits per pixel
	 * @return 0 on success
	 */
	int setMode(const gui::Size &size,gcoldepth_t bpp);

	/**
	 * Sets the cursor at given position (writes to vesa)
	 *
	 * @param pos the position
	 * @param cursor the cursor to use
	 */
	void setCursor(const gui::Pos &pos,uint cursor) {
		ui->setCursor(pos.x,pos.y,cursor);
	}

	/**
	 * Attaches the buffer <fd> to window <wid>.
	 *
	 * @param wid the window id
	 * @param fd the buffer
	 * @return 0 on success
	 */
	int joinbuf(gwinid_t wid,int fd);

	/**
	 * Attaches the given fd as event-channel to the given window
	 *
	 * @param wid the window id
	 * @param fd the fd for the event-channel
	 */
	void attach(gwinid_t wid,int fd);

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
	 * @param r the rectangle
	 * @param owner the window-owner (fd)
	 * @param style style-attributes
	 * @param titleBarHeight the height of the titlebar of that window
	 * @param title the title of the window
	 * @return the window id or an error code
	 */
	gwinid_t add(const gui::Rectangle &r,int owner,uint style,gsize_t titleBarHeight,const char *title);

	/**
	 * Removes the given window
	 *
	 * @param wid the window id
	 */
	void remove(gwinid_t wid);

	/**
	 * Determines the window at given position
	 *
	 * @param pos the position
	 * @return the window id or WINID_UNUSED
	 */
	gwinid_t getAt(const gui::Pos &pos);

	/**
	 * Retrieves information about the window with given id.
	 *
	 * @param wid the window id
	 * @param r will be set to the window rectangle
	 * @param titleBarHeight will be set to its title bar height
	 * @param style will be set to its style
	 * @return true if it succeeded
	 */
	bool getInfo(gwinid_t wid,gui::Rectangle *r,size_t *titleBarHeight,uint *style);

	/**
	 * @return the id of the active window (or WINID_UNUSED)
	 */
	gwinid_t getActive() {
		return activeWindow;
	}
	/**
	 * Makes the given window the active window. This will put it to the front and windows that
	 * are above one step behind (z-coordinate)
	 *
	 * @param wid the window id
	 * @param repaint whether the window should receive a repaint-event
	 * @param updateWinStack whether to update the window stack
	 */
	void setActive(gwinid_t wid,bool repaint,bool updateWinStack);

	/**
	 * Resizes the given window to the given size
	 *
	 * @param wid the window id
	 * @param r the rectangle
	 * @param finished whether the operation is finished
	 */
	void resize(gwinid_t wid,const gui::Rectangle &r,bool finished);

	/**
	 * Moves the given window to given position and optionally changes the size
	 *
	 * @param wid the window id
	 * @param r the rectangle
	 * @param finished whether the operation is finished
	 */
	void moveTo(gwinid_t wid,const gui::Pos &p,bool finished);

	/**
	 * Sends update-events to the window to update the given area
	 *
	 * @param wid the window id
	 * @param r the rectangle
	 * @return 0 on success
	 */
	int update(gwinid_t wid,const gui::Rectangle &r);

	/**
	 * Removes the preview rectangle, if necessary.
	 */
	void removePreview() {
		Preview::get().set(fb->addr(),gui::Rectangle(),0);
	}

	/**
	 * Notifies the UI-manager that the given rectangle has changed.
	 *
	 * @param r the rectangle
	 */
	void notifyUimng(const gui::Rectangle &r);

	/**
	 * Sends the given key event to the active window.
	 *
	 * @param data the event data
	 */
	void sendKeyEvent(const esc::UIEvents::Event &data);

	/**
	 * Sends the given mouse event to the given window
	 */
	void sendMouseEvent(gwinid_t wid,const gui::Pos &mouse,const esc::UIEvents::Event &data);

	/**
	 * Prints information about all windows to given stream.
	 *
	 * @param os the output stream
	 */
	void print(esc::OStream &os);

private:
	Window *get(gwinid_t id) {
		return windows.find(id);
	}
	Window *getActiveWin() {
		if(activeWindow != WINID_UNUSED)
			return get(activeWindow);
		return NULL;
	}
	Window *getTop();

	void remove(Window *win);
	void setActive(Window *win,bool repaint,bool updateWinStack);
	void repaint(const gui::Rectangle &r,Window *win,gpos_t z);
	void update(Window *win,const gui::Rectangle &r);
	void resetAll();
	bool validateRect(gui::Rectangle &r);
	void getRepaintRegions(std::vector<WinRect> &list,esc::DListTreap<Window>::iterator w,
		Window *win,gpos_t z,const gui::Rectangle &r);
	void clearRegion(char *mem,const gui::Rectangle &r);
	void copyRegion(char *mem,const gui::Rectangle &r,gwinid_t id);

	esc::UI *ui;
	int drvId;

	std::string theme;
	esc::Screen::Mode mode;
	esc::FrameBuffer *fb;

	gwinid_t activeWindow;
	gwinid_t topWindow;
	esc::DListTreap<Window> windows;
	static std::mutex winMutex;
	static gwinid_t nextId;
	static WinList *_inst;
};
