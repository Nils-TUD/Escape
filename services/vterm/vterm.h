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

#define INPUT_BUF_SIZE		128

#define MAX_NAME_LEN		6

/* our vterm-state */
typedef struct {
	/* identification */
	u8 index;
	tServ sid;
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
	/* wether navigation via up/down/pageup/pagedown is enabled */
	bool navigation;
	/* a backup of the screen; initially NULL */
	char *screenBackup;
	/* the current escape-state */
	u8 escapePos;
	u8 escape;
	/* the buffer for the input-stream */
	sRingBuf *inbuf;
	/* readline-buffer */
	u8 rlStartCol;
	u32 rlBufSize;
	u32 rlBufPos;
	char *rlBuffer;
	char buffer[BUFFER_SIZE];
	char titleBar[COLS * 2];
} sVTerm;

/**
 * Inits all vterms
 *
 * @param ids the service-ids
 * @return true if successfull
 */
bool vterm_initAll(tServ *ids);

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
 * @param isBreak wether it is a break-keycode
 * @param keycode the keycode
 */
void vterm_handleKeycode(bool isBreak,u32 keycode);

/**
 * Prints the given string
 *
 * @param vt the vterm
 * @param str the string
 * @param len the string-length
 * @param resetRead wether readline-stuff should be reset
 * @param readKeyboard indicates wether we should start/stop reading from keyboard
 */
void vterm_puts(sVTerm *vt,char *str,u32 len,bool resetRead,bool *readKeyboard);

#endif /* VTERM_H_ */
