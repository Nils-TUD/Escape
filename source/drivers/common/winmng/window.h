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

static const size_t WINDOW_COUNT	= 32;
static const gwinid_t WINID_UNUSED	= WINDOW_COUNT;

class Input;
class InfoDev;
class WinList;

class WinRect : public gui::Rectangle {
public:
	explicit WinRect() : gui::Rectangle(), id(WINID_UNUSED) {
	}
	explicit WinRect(const gui::Rectangle &r) : gui::Rectangle(r), id(WINID_UNUSED) {
	}

	gwinid_t id;
};

class Window : public WinRect {
	friend class Input;
	friend class InfoDev;
	friend class WinList;

public:
	enum {
		STYLE_DEFAULT				= 0,
		STYLE_POPUP					= 1,
		STYLE_DESKTOP				= 2,
	};

	/**
	 * Creates a new window
	 *
	 * @param id the window id
	 * @param x the x-coordinate
	 * @param y the y-coordinate
	 * @param z the z-coordinate
	 * @param width the width
	 * @param height the height
	 * @param owner the window-owner (fd)
	 * @param style style-attributes
	 * @param titleBarHeight the height of the titlebar of that window
	 * @param title the title of the window
	 */
	explicit Window(gwinid_t id,gpos_t x,gpos_t y,gpos_t z,gsize_t width,gsize_t height,int owner,
		uint style,gsize_t titleBarHeight,const char *title);

	/**
	 * Destroys this window
	 */
	~Window();

	/**
	 * @return the buffer (might be NULL)
	 */
	esc::FrameBuffer *getBuffer() {
		return winbuf;
	}

	/**
	 * Destroys the buffer, if existing.
	 */
	void destroybuf();

	/**
	 * Uses the file <fd> as the buffer for this window
	 *
	 * @param fd the file descriptor
	 * @return 0 on success
	 */
	int joinbuf(int fd);

	/**
	 * Attaches the given fd as event-channel to this window
	 *
	 * @param fd the fd for the event-channel
	 */
	void attach(int fd);

	/**
	 * Resizes the window to the given size
	 *
	 * @param x the x-coordinate
	 * @param y the y-coordinate
	 * @param width the new width
	 * @param height the new height
	 * @param winmng the window-manager name
	 */
	void resize(gpos_t x,gpos_t y,gsize_t width,gsize_t height);

	/**
	 * Moves the window to given position and optionally changes the size
	 *
	 * @param x the x-coordinate
	 * @param y the y-coordinate
	 * @param width the new width
	 * @param height the new height
	 */
	void moveTo(gpos_t x,gpos_t y,gsize_t width,gsize_t height);

private:
	void sendActive(bool isActive,gpos_t mouseX,gpos_t mouseY);
	void notifyWinCreate(const char *title);
	void notifyWinActive();
	void notifyWinDestroy();

	gpos_t z;
	int owner;
	int evfd;
	uint style;
	gsize_t titleBarHeight;
	esc::FrameBuffer *winbuf;
	bool ready;
};
