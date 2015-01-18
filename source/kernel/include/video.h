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

#include <assert.h>
#include <common.h>
#include <lockguard.h>
#include <ostream.h>
#include <spinlock.h>
#include <stdarg.h>

class VideoLog;

class Video : public OStream {
	friend class VideoLog;

	static const int TAB_WIDTH	= 4;

	explicit Video() : OStream(), col(0), row(0), color((BLACK << 4) | WHITE), lock() {
		// actually, we could clear the screen here, but virtualbox seems to have a bug in
		// the gdt-handling when accessing VGA memory and causing an overflow during the
		// translation from logical addresses to linear addresses.
	}

public:
	enum {
		SCREEN	= 1,
		LOG		= 2,
	};

	enum {
		BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE
	};

	/**
	 * @return the instance
	 */
	static Video &get() {
		return inst;
	}

	/**
	 * Moves the cursor to the given position
	 *
	 * @param r the row
	 * @param c the column
	 */
	void goTo(ushort r,ushort c);

	/**
	 * Backups the screen to the given buffer. Assumes (!) that the buffer is large enough to contain
	 * VID_COLS * VID_ROWS * 2 bytes!
	 *
	 * @param buffer the buffer to write to
	 * @param row pointer to the row to set
	 * @param col pointer to the col to set
	 */
	void backup(char *buffer,ushort *row,ushort *col) const;

	/**
	 * Restores the screen from the given buffer. Assumes (!) that the buffer is large enough to contain
	 * VID_COLS * VID_ROWS * 2 bytes!
	 *
	 * @param buffer the buffer to read from
	 * @param row the row to set
	 * @param col the col to set
	 */
	void restore(const char *buffer,ushort row,ushort col);

	/**
	 * Clears the screen
	 */
	void clearScreen() {
		LockGuard<SpinLock> g(&lock);
		clear();
		col = row = 0;
	}

	virtual void writec(char c);

private:
	virtual bool escape(const char **str);
	virtual uchar pipepad() const;

	void drawChar(ushort col,ushort row,char c);
	void clear();
	void move();

	static void *screen();
	static void copyScrToScr(void *dst,const void *src,size_t rows);
	static void copyScrToMem(void *dst,const void *src,size_t rows);
	static void copyMemToScr(void *dst,const void *src,size_t rows);

	ulong col;
	ulong row;
	uchar color;
	mutable SpinLock lock;
	static Video inst;
};

#if defined(__x86__)
#	include <arch/x86/video.h>
#elif defined(__eco32__)
#	include <arch/eco32/video.h>
#elif defined(__mmix__)
#	include <arch/mmix/video.h>
#endif

inline void Video::goTo(ushort r,ushort c) {
	assert(r < VID_ROWS && c < VID_COLS);
	col = c;
	row = r;
}
