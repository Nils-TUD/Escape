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

#ifndef VTERM_H_
#define VTERM_H_

#include <esc/common.h>

#define COLS				80
#define ROWS				25
#define TAB_WIDTH			4

#define HISTORY_SIZE		(ROWS * 8)
#define BUFFER_SIZE			(COLS * 2 * HISTORY_SIZE)

#define MAX_NAME_LEN		6

/* the header for the set-screen message */
typedef struct {
	sMsgHeader header;
	u16 startPos;
} __attribute__((packed)) sMsgVidSetScr;

/* our vterm-state */
typedef struct {
	/* identification */
	u8 index;
	char name[MAX_NAME_LEN + 1];
	/* position (on the current page) */
	u8 col;
	u8 row;
	/* colors */
	u8 foreground;
	u8 background;
	/* wether this vterm is currently active */
	bool active;
	/* file-descriptors */
	tFD video;
	tFD speaker;
	tFD self;
	/* the first line with content */
	u16 firstLine;
	/* the line where row+col starts */
	u16 currLine;
	/* the first visible line */
	u16 firstVisLine;
	/* the used keymap */
	u16 keymap;
	/* wether entered characters should be echo'd to screen */
	bool echo;
	/* wether the vterm should read until a newline occurrs */
	bool readLine;
	/* readline-buffer */
	u8 rlStartCol;
	u32 rlBufSize;
	u32 rlBufPos;
	char *rlBuffer;
	/* in message form for performance-issues */
	struct {
		sMsgVidSetScr header;
		char data[BUFFER_SIZE];
	} __attribute__((packed)) buffer;
	struct {
		sMsgVidSetScr header;
		char data[COLS * 2];
	} __attribute__((packed)) titleBar;
} sVTerm;

/**
 * Inits all vterms
 *
 * @return true if successfull
 */
bool vterm_initAll(void);

/**
 * @param index the index
 * @return the vterm with given index
 */
sVTerm *vterm_get(u32 index);

/**
 * Selects the given vterminal
 *
 * @param index the index of the vterm
 */
void vterm_selectVTerm(u32 index);

/**
 * Releases resources
 *
 * @param vt the vterm
 */
void vterm_destroy(sVTerm *vt);

/**
 * Handles the given keycode for the active vterm
 *
 * @param msg the message from the keyboard-driver
 */
void vterm_handleKeycode(sMsgKbResponse *msg);

/**
 * Prints the given string
 *
 * @param vt the vterm
 * @param str the string
 * @param resetRead wether readline-stuff should be reset
 */
void vterm_puts(sVTerm *vt,char *str,bool resetRead);

#endif /* VTERM_H_ */
