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

#include <esc/common.h>
#include <esc/messages.h>
#include <esc/mem.h>
#include <ipc/proto/default.h>
#include <vthrow.h>
#include <vector>

namespace ipc {

/**
 * The IPC-interface for all screen-devices (vga, vesa, uimng, ...). It provides mode-getting and
 * setting, screen updates and cursor setting.
 */
class Screen {
public:
	struct Mode {
		int id;
		uint cols;
		uint rows;
		uint width;
		uint height;
		uchar bitsPerPixel;
		uchar redMaskSize;				/* Size of direct color red mask   */
		uchar redFieldPosition;			/* Bit posn of lsb of red mask     */
		uchar greenMaskSize;			/* Size of direct color green mask */
		uchar greenFieldPosition;		/* Bit posn of lsb of green mask   */
		uchar blueMaskSize;				/* Size of direct color blue mask  */
		uchar blueFieldPosition;		/* Bit posn of lsb of blue mask    */
		uintptr_t physaddr;
		size_t tuiHeaderSize;
		size_t guiHeaderSize;
		int mode;
		int type;
	};

	/* the modes */
	enum {
		MODE_TEXT,
		MODE_GRAPHICAL,
	};

	/* the mode types */
	enum {
		MODE_TYPE_TUI	= 1,
		MODE_TYPE_GUI	= 2,
	};

	/* cursors */
	enum {
		CURSOR_DEFAULT,
		CURSOR_RESIZE_L,
		CURSOR_RESIZE_BR,
		CURSOR_RESIZE_VERT,
		CURSOR_RESIZE_BL,
		CURSOR_RESIZE_R
	};

	static const size_t CURSOR_RESIZE_WIDTH	= 6;

	/**
	 * Opens the given device
	 *
	 * @param path the path to the device
	 * @throws if the open failed
	 */
	explicit Screen(const char *path) : _is(path) {
	}
	/**
	 * Attaches this object to the given file-descriptor
	 *
	 * @param fd
	 */
	explicit Screen(int f) : _is(f) {
	}

	/**
	 * @return the file-descriptor
	 */
	int fd() const throw() {
		return _is.fd();
	}

	/**
	 * Sets the cursor to the given position and shape. Note that you receive no response for this
	 * call. So it is not guaranteed that the operation is finished after the call returns.
	 *
	 * @pre setMode() has to be called before to share the framebuffer
	 * @throws if the send failed
	 */
	void setCursor(gpos_t x,gpos_t y,int cursor = 0) {
		_is << x << y << cursor << Send(MSG_SCR_SETCURSOR);
	}

	/**
	 * Updates the given rectangle of the screen from the established framebuffer. Note that you
	 * receive no response for this call. So it is not guaranteed that the operation is finished
	 * after the call returns.
	 *
	 * @param x the x-position
	 * @param y the y-position
	 * @param width the width of the rectangle
	 * @param height the height of the rectangle
	 * @pre setMode() has to be called before to share the framebuffer
	 * @throws if the send failed
	 */
	void update(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		_is << x << y << width << height << Send(MSG_SCR_UPDATE);
	}

	/**
	 * @return the currently set mode
	 * @throws if the operation failed
	 */
	Mode getMode() {
		Mode mode;
		int res;
		_is << SendReceive(MSG_SCR_GETMODE) >> res >> mode;
		if(res < 0)
			VTHROWE("getMode()",res);
		return mode;
	}

	/**
	 * Sets the given mode and shares the shared-memory <shm> with the device.
	 *
	 * @param type the screen type (ipc::Screen::MODE_TYPE_{TUI,GUI})
	 * @param mode the mode-id to set
	 * @param shm the shared-memory file used as the framebuffer
	 * @param switchMode whether to actually set the given mode
	 * @throws if the operation failed
	 */
	void setMode(int type,int mode,const char *shm,bool switchMode) {
		int res;
		_is << mode << type << switchMode << CString(shm) << SendReceive(MSG_SCR_SETMODE) >> res;
		if(res < 0)
			VTHROWE("setMode(" << type << "," << mode << "," << shm << ")",res);
	}

	/**
	 * Determines the number of available modes.
	 *
	 * @return the number of modes
	 * @throws if the operation failed
	 */
	size_t getModeCount() {
		ssize_t count;
		_is << static_cast<size_t>(0) << SendReceive(MSG_SCR_GETMODES) >> count;
		if(count < 0)
			VTHROWE("getModeCount()",count);
		return count;
	}

	/**
	 * Determines the list of available modes.
	 *
	 * @return the list of modes
	 * @throws if the operation failed
	 */
	std::vector<Mode> getModes();

	/**
	 * Tries to find a good matching text mode for the given number of columns and rows.
	 *
	 * @param cols the number of columns
	 * @param rows the number of rows
	 * @return the mode
	 * @throws if the operation failed
	 */
	Mode findTextMode(uint cols,uint rows) {
		std::vector<Mode> modes = getModes();
		if(modes.size() == 0)
			throw std::default_error("No modes found");
		return findTextModeIn(modes,cols,rows);
	}
	/**
	 * Tries to find a good matching text mode for the given number of columns and rows in the
	 * given mode-list.
	 *
	 * @param modes the mode-list
	 * @param cols the number of columns
	 * @param rows the number of rows
	 * @return the mode
	 * @throws if the operation failed
	 */
	Mode findTextModeIn(const std::vector<Mode> &modes,uint cols,uint rows);

	/**
	 * Tries to find a good matching graphics mode for the given width, height and color-depth.
	 *
	 * @param width the width
	 * @param height the height
	 * @param bpp the color-depth
	 * @return the mode
	 * @throws if the operation failed
	 */
	Mode findGraphicsMode(gsize_t width,gsize_t height,gcoldepth_t bpp) {
		std::vector<Mode> modes = getModes();
		if(modes.size() == 0)
			throw std::default_error("No modes found");
		return findGraphicsModeIn(modes,width,height,bpp);
	}
	/**
	 * Tries to find a good matching graphics mode for the given number of columns and rows in the
	 * given mode-list.
	 *
	 * @param modes the mode-list
	 * @param width the width
	 * @param height the height
	 * @param bpp the color-depth
	 * @return the mode
	 * @throws if the operation failed
	 */
	Mode findGraphicsModeIn(const std::vector<Mode> &modes,gsize_t width,gsize_t height,gcoldepth_t bpp);

protected:
	IPCStream _is;
};

/**
 * Represents a framebuffer which is intended to be used in conjunction with Screen
 */
class FrameBuffer {
public:
	/**
	 * Joines the framebuffer represented as the file <name>.
	 *
	 * @param m the mode to use
	 * @param name the shared-memory file
	 * @param type the screen type (ipc::Screen::MODE_TYPE_{TUI,GUI})
	 * @throws if the operation failed
	 */
	explicit FrameBuffer(const Screen::Mode &m,const char *file,int type)
		: _mode(m), _filename(file),
		  _addr(init(_mode,file,type,O_RDWR,0)), _created(false) {
	}
	/**
	 * Creates the framebuffer represented as the file <name>.
	 *
	 * @param m the mode to use
	 * @param file the shared-memory file
	 * @param type the screen type (ipc::Screen::MODE_TYPE_{TUI,GUI})
	 * @param perms the permissions to give to the file
	 * @throws if the operation failed
	 */
	explicit FrameBuffer(const Screen::Mode &m,const char *file,int type,uint perms)
		: _mode(m), _filename(file),
		  _addr(init(_mode,file,type,O_RDWR | O_CREAT,perms)), _created(true) {
	}

	/**
	 * Releases up all the resources, i.e. unmaps the memory and deletes the file, if it was created.
	 */
	~FrameBuffer();

	/**
	 * @return the address of the framebuffer
	 */
	char *addr() throw() {
		return _addr;
	}
	/**
	 * @return the mode
	 */
	const Screen::Mode &mode() const throw() {
		return _mode;
	}
	/**
	 * @return the filename
	 */
	const std::string filename() const throw() {
		return _filename;
	}

	/**
	 * Renames the shared memory file to <newName>.
	 *
	 * @param newName the new filename
	 * @throws if the operation failed
	 */
	void rename(const std::string &newName) {
		int res;
		if((res = shm_rename(_filename.c_str(),newName.c_str())) < 0)
			VTHROWE("shm_rename(" << _filename << "," << newName << ")",res);
		_filename = newName;
	}

private:
	static char *init(Screen::Mode &mode,const char *file,int type,int flags,uint perms);

	Screen::Mode _mode;
	std::string _filename;
	char *_addr;
	bool _created;
};

}
