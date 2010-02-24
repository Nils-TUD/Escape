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

#ifndef VIDEO_H_
#define VIDEO_H_

#include <common.h>
#include <stdarg.h>

typedef enum {BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE} eColor;

#define VID_COLS				80
#define VID_ROWS				25

/**
 * Inits the video-stuff
 */
void vid_init(void);

/**
 * Prints the given character to the current position on the screen
 *
 * @param c the character
 */
void vid_putchar(char c);

/**
 * The kernel-version of printf. Currently it supports:
 * %d: signed integer
 * %u: unsigned integer, base 10
 * %o: unsigned integer, base 8
 * %x: unsigned integer, base 16 (small letters)
 * %X: unsigned integer, base 16 (big letters)
 * %b: unsigned integer, base 2
 * %s: string
 * %c: character
 *
 * @param fmt the format
 */
void vid_printf(const char *fmt,...);

/**
 * Same as vid_printf, but with the va_list as argument
 *
 * @param fmt the format
 * @param ap the argument-list
 */
void vid_vprintf(const char *fmt,va_list ap);

#endif
