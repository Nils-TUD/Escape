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

#ifndef VTCTRL_H_
#define VTCTRL_H_

#include <esc/common.h>
#include <esc/ringbuffer.h>
#include <esc/esccodes.h>
#include <esc/messages.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAB_WIDTH			4
#define HISTORY_SIZE		12
#define INPUT_BUF_SIZE		512
#define MAX_VT_NAME_LEN		15

/**
 * The handler for shortcuts
 */
typedef struct sVTerm sVTerm;
typedef bool (*fHandleShortcut)(sVTerm *vt,uchar keycode,uchar modifier,char c);
typedef void (*fSetCursor)(sVTerm *vt);

/* global configuration */
typedef struct {
	bool readKb;
	bool enabled;
} sVTermCfg;

/* our vterm-state */
struct sVTerm {
	/* identification */
	uchar index;
	int sid;
	char name[MAX_VT_NAME_LEN + 1];
	/* function-pointers */
	fSetCursor setCursor;
	/* number of cols/rows on the screen */
	uchar cols;
	uchar rows;
	/* position (on the current page) */
	uchar col;
	uchar row;
	/* colors */
	uchar defForeground;
	uchar defBackground;
	uchar foreground;
	uchar background;
	/* whether this vterm is currently active */
	uchar active;
	/* file-descriptors */
	int video;
	int speaker;
	/* the first line with content */
	ushort firstLine;
	/* the line where row+col starts */
	ushort currLine;
	/* the first visible line */
	ushort firstVisLine;
	/* a range that should be updated */
	ushort upStart;
	ushort upLength;
	short upScroll;
	/* whether entered characters should be echo'd to screen */
	uchar echo;
	/* whether the vterm should read until a newline occurrs */
	uchar readLine;
	/* whether navigation via up/down/pageup/pagedown is enabled */
	uchar navigation;
	/* whether all output should be printed into the readline-buffer */
	uchar printToRL;
	/* whether all output should be printed to COM1 */
	uchar printToCom1;
	/* a backup of the screen; initially NULL */
	char *screenBackup;
	ushort backupCol;
	ushort backupRow;
	/* the buffer for the input-stream */
	uchar inbufEOF;
	sRingBuf *inbuf;
	/* the pid of the shell for ctrl+c notifications */
	pid_t shellPid;
	/* the escape-state */
	int escapePos;
	char escapeBuf[MAX_ESCC_LENGTH];
	/* readline-buffer */
	uchar rlStartCol;
	size_t rlBufSize;
	size_t rlBufPos;
	char *rlBuffer;
	char *buffer;
	char *titleBar;
};

/* the colors */
typedef enum {
	/* 0 */ BLACK,
	/* 1 */ BLUE,
	/* 2 */ GREEN,
	/* 3 */ CYAN,
	/* 4 */ RED,
	/* 5 */ MARGENTA,
	/* 6 */ ORANGE,
	/* 7 */ LIGHTGRAY,
	/* 8 */ GRAY,
	/* 9 */ LIGHTBLUE,
	/* 10 */ LIGHTGREEN,
	/* 11 */ LIGHTCYAN,
	/* 12 */ LIGHTRED,
	/* 13 */ LIGHTMARGENTA,
	/* 14 */ YELLOW,
	/* 15 */ WHITE
} eColor;

/**
 * Inits the vterm
 *
 * @param vt the vterm
 * @param vidSize the size of the screen
 * @param vidFd the file-descriptor for the video-driver (or whatever you need :))
 * @param speakerFd the file-descriptor for the speaker-driver
 * @return true if successfull
 */
bool vterm_init(sVTerm *vt,sVTSize *vidSize,int vidFd,int speakerFd);

/**
 * Handles the control-commands
 *
 * @param vt the vterm
 * @param cfg global configuration
 * @param cmd the command
 * @param data the data
 * @return the result
 */
int vterm_ctl(sVTerm *vt,sVTermCfg *cfg,uint cmd,void *data);

/**
 * Scrolls the screen by <lines> up (positive) or down (negative)
 *
 * @param vt the vterm
 * @param lines the number of lines to move
 */
void vterm_scroll(sVTerm *vt,int lines);

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
void vterm_markDirty(sVTerm *vt,ushort start,size_t length);

/**
 * Releases resources
 *
 * @param vt the vterm
 */
void vterm_destroy(sVTerm *vt);

#ifdef __cplusplus
}
#endif

#endif /* VTCTRL_H_ */
