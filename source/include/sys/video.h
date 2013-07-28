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

#include <sys/common.h>
#include <sys/printf.h>
#include <sys/spinlock.h>
#include <assert.h>
#include <stdarg.h>

class Video {
	Video() = delete;

	static const int TAB_WIDTH	= 4;

public:
	enum {
		SCREEN	= 1,
		LOG		= 2,
	};

	enum {
		BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE
	};

	/**
	 * Inits the video-stuff
	 */
	static void init() {
		clearScreen();
	}

	/**
	 * Moves the cursor to the given position
	 *
	 * @param r the row
	 * @param c the column
	 */
	static void goTo(ushort r,ushort c);

	/**
	 * Backups the screen to the given buffer. Assumes (!) that the buffer is large enough to contain
	 * VID_COLS * VID_ROWS * 2 bytes!
	 *
	 * @param buffer the buffer to write to
	 * @param row pointer to the row to set
	 * @param col pointer to the col to set
	 */
	static void backup(char *buffer,ushort *row,ushort *col);

	/**
	 * Restores the screen from the given buffer. Assumes (!) that the buffer is large enough to contain
	 * VID_COLS * VID_ROWS * 2 bytes!
	 *
	 * @param buffer the buffer to read from
	 * @param row the row to set
	 * @param col the col to set
	 */
	static void restore(const char *buffer,ushort row,ushort col);

	/**
	 * Sets the targets of the printing
	 *
	 * @param ntargets the targets (TARGET_*)
	 */
	static void setTargets(uint ntargets) {
		targets = ntargets;
	}

	/**
	 * Clears the screen
	 */
	static void clearScreen() {
		spinlock_aquire(&lock);
		clear();
		col = row = 0;
		spinlock_release(&lock);
	}

	/**
	 * Sets the given function as callback for every character to print (instead of the default one
	 * for printing to screen)
	 *
	 * @param func the function
	 */
	static void setPrintFunc(fPrintc func) {
		printFunc = func;
	}

	/**
	 * Resets the print-function to the default one
	 */
	static void unsetPrintFunc() {
		printFunc = putchar;
	}

	/**
	 * Formatted output to the video-screen
	 *
	 * @param fmt the format
	 */
	static void printf(const char *fmt,...) asm("vid_printf");

	/**
	 * Same as printf, but with the va_list as argument
	 *
	 * @param fmt the format
	 * @param ap the argument-list
	 */
	static void vprintf(const char *fmt,va_list ap) asm("vid_vprintf");

private:
	static void *screen();
	static void putchar(char c);
	static void drawChar(ushort col,ushort row,char c);
	static void copyScrToScr(void *dst,const void *src,size_t rows);
	static void copyScrToMem(void *dst,const void *src,size_t rows);
	static void copyMemToScr(void *dst,const void *src,size_t rows);
	static void clear();
	static void move();
	static uchar handlePipePad();
	static void handleColorCode(const char **str);

	static fPrintc printFunc;
	static ulong col;
	static ulong row;
	static uchar color;
	static ulong targets;
	static bool lastWasLineStart;
	static klock_t lock;
};

#ifdef __i386__
#include <sys/arch/i586/video.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/video.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/video.h>
#endif

inline void Video::goTo(ushort r,ushort c) {
	assert(r < VID_ROWS && c < VID_COLS);
	col = c;
	row = r;
}

inline uchar Video::handlePipePad() {
	return VID_COLS - col;
}
