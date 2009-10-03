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
#include <ringbuffer.h>
#include <esccodes.h>

#define TAB_WIDTH			4
#define HISTORY_SIZE		12
#define INPUT_BUF_SIZE		128
#define MAX_VT_NAME_LEN		6

/* our vterm-state */
typedef struct {
	/* identification */
	u8 index;
	tServ sid;
	char name[MAX_VT_NAME_LEN + 1];
	/* number of cols/rows on the screen */
	u8 cols;
	u8 rows;
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
	/* a range that should be updated */
	u16 upStart;
	u16 upLength;
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
	/* the buffer for the input-stream */
	sRingBuf *inbuf;
	/* the pid of the shell for ctrl+c notifications */
	tPid shellPid;
	/* the escape-state */
	s32 escapePos;
	char escapeBuf[MAX_ESCC_LENGTH];
	/* readline-buffer */
	u8 rlStartCol;
	u32 rlBufSize;
	u32 rlBufPos;
	char *rlBuffer;
	char *buffer;
	char *titleBar;
} sVTerm;

/* the colors */
typedef enum {
	/* 0 */ BLACK,
	/* 1 */ BLUE,
	/* 2 */ GREEN,
	/* 3 */ CYAN,
	/* 4 */ RED,
	/* 5 */ MARGENTA,
	/* 6 */ ORANGE,
	/* 7 */ WHITE,
	/* 8 */ GRAY,
	/* 9 */ LIGHTBLUE
} eColor;

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
 * @return the currently active vterm
 */
sVTerm *vterm_getActive(void);

/**
 * Sets the cursor
 *
 * @param vt the vterm
 */
void vterm_setCursor(sVTerm *vt);

/**
 * Handles the ioctl-command
 *
 * @param vt the vterm
 * @param cmd the command
 * @param data the data
 * @param readKb may be changed during ioctl
 * @return the result
 */
s32 vterm_ioctl(sVTerm *vt,u32 cmd,void *data,bool *readKb);

/**
 * Scrolls the screen by <lines> up (positive) or down (negative)
 *
 * @param vt the vterm
 * @param lines the number of lines to move
 */
void vterm_scroll(sVTerm *vt,s16 lines);

/**
 * Marks the whole screen including title-bar dirty
 *
 * @param vt the vterm
 */
void vterm_markScrDirty(sVTerm *vt);

/**
 * Marks the given range as dirty
 *
 * @param vt the vterm
 * @param start the start-position
 * @param length the number of bytes
 */
void vterm_markDirty(sVTerm *vt,u16 start,u16 length);

/**
 * Updates the dirty range
 *
 * @param vt the vterm
 */
void vterm_update(sVTerm *vt);

/**
 * Releases resources
 *
 * @param vt the vterm
 */
void vterm_destroy(sVTerm *vt);

#endif /* VTERM_H_ */
