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

#include <esc/common.h>
#include <messages.h>
#include <esc/io.h>
#include <esc/ports.h>
#include <esc/proc.h>
#include <esc/keycodes.h>
#include <esc/heap.h>
#include <esc/signals.h>
#include <esc/service.h>
#include <esc/fileio.h>
#include <string.h>
#include <ringbuffer.h>

#include "vterm.h"
#include "keymap.h"
#include "keymap.us.h"
#include "keymap.ger.h"

/* the number of lines to keep in history */
#define INITIAL_RLBUF_SIZE	50
#define RLBUF_INCR			20

/* the number of left-shifts for each state */
#define STATE_SHIFT			0
#define STATE_CTRL			1
#define STATE_ALT			2

#define TITLE_BAR_COLOR		0x90
#define OS_TITLE			"E\x90" \
							"s\x90" \
							"c\x90" \
							"a\x90" \
							"p\x90" \
							"e\x90" \
							" \x90" \
							"v\x90" \
							"0\x90" \
							".\x90" \
							"1\x90"

typedef sKeymapEntry *(*fKeymapGet)(u8 keyCode);

/* the colors */
typedef enum {BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE} eColor;

/**
 * Inits the vterm
 *
 * @param vt the vterm
 * @return true if successfull
 */
static bool vterm_init(sVTerm *vt);

/**
 * Sends the character at given position to the video-driver
 *
 * @param vt the vterm
 * @param row the row
 * @param col the col
 */
static void vterm_sendChar(sVTerm *vt,u8 row,u8 col);

/**
 * Sets the cursor
 *
 * @param vt the vterm
 */
static void vterm_setCursor(sVTerm *vt);

/**
 * Prints the given character to screen
 *
 * @param vt the vterm
 * @param c the character
 */
static void vterm_putchar(sVTerm *vt,char c);

/**
 * Inserts a new line
 *
 * @param vt the vterm
 */
static void vterm_newLine(sVTerm *vt);

/**
 * Scrolls the screen by <lines> up (positive) or down (negative)
 *
 * @param vt the vterm
 * @param lines the number of lines to move
 */
static void vterm_scroll(sVTerm *vt,s16 lines);

/**
 * Refreshes the screen
 *
 * @param vt the vterm
 */
static void vterm_refreshScreen(sVTerm *vt);

/**
 * Refreshes <count> lines beginning with <start>
 *
 * @param vt the vterm
 * @param start the start-line
 * @param count the number of lines
 */
static void vterm_refreshLines(sVTerm *vt,u16 start,u16 count);

/**
 * Handles the escape-code
 *
 * @param vt the vterm
 * @param keycode the keycode of the escape
 * @param value the value of the escape
 * @param stopRead will be set to read or stop reading from keyboard
 * @return true if something has been done
 */
static bool vterm_handleEscape(sVTerm *vt,u8 keycode,u8 value,bool *readKeyboard);

/**
 * Flushes the readline-buffer
 *
 * @param vt the vterm
 */
static void vterm_rlFlushBuf(sVTerm *vt);

/**
 * Puts the given charactern into the readline-buffer and handles everything necessary
 *
 * @param vt the vterm
 * @param c the character
 */
static void vterm_rlPutchar(sVTerm *vt,char c);

/**
 * @param vt the vterm
 * @return the current position in the readline-buffer
 */
static u32 vterm_rlGetBufPos(sVTerm *vt);

/**
 * Handles the given keycode for readline
 *
 * @param vt the vterm
 * @param keycode the keycode
 * @return true if handled
 */
static bool vterm_rlHandleKeycode(sVTerm *vt,u8 keycode);

static sMsg msg;

/* our keymaps */
static fKeymapGet keymaps[] = {
	keymap_us_get,
	keymap_ger_get
};
/* vterms */
static sVTerm vterms[VTERM_COUNT];
static sVTerm *activeVT = NULL;

/* key states */
static bool shiftDown;
static bool altDown;
static bool ctrlDown;

bool vterm_initAll(tServ *ids) {
	char name[MAX_NAME_LEN + 1];
	u32 i;

	for(i = 0; i < VTERM_COUNT; i++) {
		vterms[i].index = i;
		vterms[i].sid = ids[i];
		sprintf(name,"vterm%d",i);
		memcpy(vterms[i].name,name,MAX_NAME_LEN + 1);
		if(!vterm_init(vterms + i))
			return false;
	}

	shiftDown = false;
	altDown = false;
	ctrlDown = false;
	return true;
}

sVTerm *vterm_get(u32 index) {
	return vterms + index;
}

static bool vterm_init(sVTerm *vt) {
	tFD vidFd,speakerFd;
	u32 i,len;
	char *ptr,*s;

	/* open video */
	vidFd = open("/drivers/video",IO_WRITE);
	if(vidFd < 0) {
		printe("Unable to open '/services/video'");
		return false;
	}

	/* open speaker */
	speakerFd = open("/services/speaker",IO_WRITE);
	if(speakerFd < 0) {
		printe("Unable to open '/services/speaker'");
		return false;
	}

	/* init state */
	vt->col = 0;
	vt->row = ROWS - 1;
	vt->foreground = WHITE;
	vt->background = BLACK;
	vt->active = false;
	vt->video = vidFd;
	vt->speaker = speakerFd;
	/* start on first line of the last page */
	vt->firstLine = HISTORY_SIZE - ROWS;
	vt->currLine = HISTORY_SIZE - ROWS;
	vt->firstVisLine = HISTORY_SIZE - ROWS;
	/* default behaviour */
	vt->echo = true;
	vt->readLine = true;
	vt->navigation = true;
	vt->keymap = 1;
	vt->escapePos = 0;
	vt->rlStartCol = 0;
	vt->rlBufSize = INITIAL_RLBUF_SIZE;
	vt->rlBufPos = 0;
	vt->rlBuffer = (char*)malloc(vt->rlBufSize * sizeof(char));
	if(vt->rlBuffer == NULL) {
		printe("Unable to allocate memory for vterm-buffer");
		return false;
	}

	vt->inbuf = rb_create(sizeof(char),INPUT_BUF_SIZE,RB_OVERWRITE);
	if(vt->inbuf == NULL) {
		printe("Unable to allocate memory for ring-buffer");
		return false;
	}

	/* fill buffer with spaces to ensure that the cursor is visible (spaces, white on black) */
	ptr = vt->buffer;
	for(i = 0; i < BUFFER_SIZE; i += 4) {
		*ptr++ = 0x20;
		*ptr++ = 0x07;
		*ptr++ = 0x20;
		*ptr++ = 0x07;
	}

	/* build title bar */
	ptr = vt->titleBar;
	s = vt->name;
	for(i = 0; *s; i++) {
		*ptr++ = *s++;
		*ptr++ = TITLE_BAR_COLOR;
	}
	for(; i < COLS; i++) {
		*ptr++ = ' ';
		*ptr++ = TITLE_BAR_COLOR;
	}
	len = strlen(OS_TITLE);
	i = (((COLS * 2) / 2) - (len / 2)) & ~0x1;
	ptr = vt->titleBar;
	memcpy(ptr + i,OS_TITLE,len);
	return true;
}

void vterm_selectVTerm(u32 index) {
	sVTerm *vt = vterms + index;
	if(activeVT != NULL)
		activeVT->active = false;
	vt->active = true;
	activeVT = vt;

	/* refresh screen and write titlebar */
	seek(vt->video,0,SEEK_SET);
	write(vt->video,vt->titleBar,sizeof(u16) * COLS * 2);
	vterm_refreshScreen(vt);
	vterm_setCursor(vt);
}

void vterm_destroy(sVTerm *vt) {
	free(vt->rlBuffer);
	close(vt->video);
	close(vt->speaker);
}

void vterm_puts(sVTerm *vt,char *str,u32 len,bool resetRead,bool *readKeyboard) {
	char c;
	char *strBegin = str;
	u32 oldFirstLine = vt->firstLine;
	u32 newPos,oldPos = vt->row * COLS + vt->col;
	u32 start,count;

	if(vt->escapePos > 0) {
		u8 keycode,modifier;
		u32 rem = len - (str - strBegin);
		if(vt->escapePos == 1 && rem < 2) {
			vt->escape = *(u8*)str;
			vt->escapePos = 2;
			return;
		}

		if(vt->escapePos == 1) {
			keycode = *(u8*)str;
			modifier = *((u8*)str + 1);
			str += 2;
		}
		else {
			keycode = vt->escape;
			modifier = *(u8*)str;
			str++;
		}
		vterm_handleEscape(vt,keycode,modifier,readKeyboard);
		vt->escapePos = 0;
	}

	while((c = *str)) {
		if(c == '\033') {
			str++;
			u32 rem = len - (str - strBegin);
			/* escape not finished? */
			if(rem < 2) {
				if(rem == 0)
					vt->escapePos = 1;
				else {
					vt->escapePos = 2;
					vt->escape = *(u8*)str;
				}
				break;
			}
			vterm_handleEscape(vt,*(u8*)str,*((u8*)str + 1),readKeyboard);
			str += 2;
			continue;
		}
		vterm_putchar(vt,c);
		str++;
	}

	/* scroll to current line, if necessary */
	if(vt->firstVisLine != vt->currLine)
		vterm_scroll(vt,vt->firstVisLine - vt->currLine);

	if(vt->active) {
		/* so refresh all lines that need to be refreshed. thats faster than sending all
		 * chars individually */
		newPos = vt->row * COLS + vt->col;
		start = oldPos / COLS;
		count = ((newPos - oldPos) + COLS - 1) / COLS;
		count += oldFirstLine - vt->firstLine;
		if(count <= 0)
			count = 1;
		vterm_refreshLines(vt,start,count);
		vterm_setCursor(vt);
	}

	/* reset reading */
	if(resetRead) {
		vt->rlBufPos = 0;
		vt->rlStartCol = vt->col;
	}
}

static void vterm_sendChar(sVTerm *vt,u8 row,u8 col) {
	char *ptr = vt->buffer + (vt->currLine * COLS * 2) + (row * COLS * 2) + (col * 2);

	/* scroll to current line, if necessary */
	if(vt->firstVisLine != vt->currLine)
		vterm_scroll(vt,vt->firstVisLine - vt->currLine);

	/* write last character to video-driver */
	if(vt->active) {
		seek(vt->video,row * COLS * 2 + col * 2,SEEK_SET);
		write(vt->video,ptr,2);
	}
}

static void vterm_setCursor(sVTerm *vt) {
	if(vt->active) {
		sIoCtlCursorPos pos;
		pos.col = vt->col;
		pos.row = vt->row;
		ioctl(vt->video,IOCTL_VID_SETCURSOR,(u8*)&pos,sizeof(pos));
	}
}

static void vterm_putchar(sVTerm *vt,char c) {
	u32 i;

	/* move all one line up, if necessary */
	if(vt->row >= ROWS) {
		vterm_newLine(vt);
		vterm_refreshScreen(vt);
		vt->row--;
	}

	/* write to bochs(0xe9) / qemu(0x3f8) console */
	/* a few characters don't make much sense here */
	if(c != '\r' && c != '\a' && c != '\b' && c != '\t') {
		outByte(0xe9,c);
		outByte(0x3f8,c);
		while((inByte(0x3fd) & 0x20) == 0);
	}

	switch(c) {
		case '\n':
			/* to next line */
			vt->row++;
			/* move cursor to line start */
			vterm_putchar(vt,'\r');
			break;

		case '\r':
			/* to line-start */
			vt->col = 0;
			break;

		case '\a':
			/* beep */
			msg.args.arg1 = 1000;
			msg.args.arg2 = 1;
			send(vt->speaker,MSG_SPEAKER_BEEP,&msg,sizeof(msg.args));
			break;

		case '\b':
			if((!vt->readLine && vt->col > 0) || (vt->readLine && vt->rlBufPos > 0)) {
				if(!vt->readLine || vt->echo) {
					i = (vt->currLine * COLS * 2) + (vt->row * COLS * 2) + (vt->col * 2);
					/* move the characters back in the buffer */
					memmove(vt->buffer + i - 2,vt->buffer + i,(COLS - vt->col) * 2);
					vt->col--;
				}

				if(vt->readLine) {
					vt->rlBuffer[vt->rlBufPos] = '\0';
					vt->rlBufPos--;
				}

				/* overwrite line */
				vterm_refreshLines(vt,vt->row,1);
			}
			else {
				/* beep */
				msg.args.arg1 = 1000;
				msg.args.arg2 = 1;
				send(vt->speaker,MSG_SPEAKER_BEEP,&msg,sizeof(msg.args));
			}
			break;

		case '\t':
			i = TAB_WIDTH - vt->col % TAB_WIDTH;
			while(i-- > 0) {
				vterm_putchar(vt,' ');
			}
			break;

		default: {
			/* do an explicit newline if necessary */
			if(vt->col >= COLS)
				vterm_putchar(vt,'\n');

			i = (vt->currLine * COLS * 2) + (vt->row * COLS * 2) + (vt->col * 2);

			/* write to buffer */
			vt->buffer[i] = c;
			vt->buffer[i + 1] = (vt->background << 4) | vt->foreground;

			vt->col++;
		}
		break;
	}
}

static void vterm_newLine(sVTerm *vt) {
	char *src,*dst;
	u32 i,count = (HISTORY_SIZE - vt->firstLine) * COLS * 2;
	/* move one line back */
	if(vt->firstLine > 0) {
		dst = vt->buffer + ((vt->firstLine - 1) * COLS * 2);
		vt->firstLine--;
	}
	/* overwrite first line */
	else
		dst = vt->buffer + (vt->firstLine * COLS * 2);
	src = dst + COLS * 2;
	memmove(dst,src,count);

	/* clear last line */
	dst = vt->buffer + (vt->currLine + vt->row - 1) * COLS * 2;
	for(i = 0; i < COLS * 2; i += 4) {
		*dst++ = 0x20;
		*dst++ = 0x07;
		*dst++ = 0x20;
		*dst++ = 0x07;
	}
}

static void vterm_scroll(sVTerm *vt,s16 lines) {
	u16 old = vt->firstVisLine;
	if(lines > 0) {
		/* ensure that we don't scroll above the first line with content */
		vt->firstVisLine = MAX(vt->firstLine,(s16)vt->firstVisLine - lines);
	}
	else {
		/* ensure that we don't scroll behind the last line */
		vt->firstVisLine = MIN(HISTORY_SIZE - ROWS,vt->firstVisLine - lines);
	}

	if(vt->active && old != vt->firstVisLine)
		vterm_refreshScreen(vt);
}

static void vterm_refreshScreen(sVTerm *vt) {
	vterm_refreshLines(vt,1,ROWS - 1);
}

static void vterm_refreshLines(sVTerm *vt,u16 start,u16 count) {
	u32 amount,done = 0;
	if(!vt->active)
		return;

	seek(vt->video,start * COLS * 2,SEEK_SET);
	/* send messages (take care of msg-size) */
	while(done < count) {
		amount = MIN(count,sizeof(msg.data.d) / (COLS * 2));
		write(vt->video,vt->buffer + (vt->firstVisLine + start) * COLS * 2,amount * COLS * 2);

		done += amount;
		start += amount;
	}
}

static bool vterm_handleEscape(sVTerm *vt,u8 keycode,u8 value,bool *readKeyboard) {
	bool res = false;
	switch(keycode) {
		case VK_UP:
			if(vt->row > 1)
				vt->row--;
			break;
		case VK_DOWN:
			if(vt->row < ROWS - 1)
				vt->row++;
			break;
		case VK_LEFT:
			if(vt->col > 0)
				vt->col--;
			res = true;
			break;
		case VK_RIGHT:
			if(vt->col < COLS - 1)
				vt->col++;
			res = true;
			break;
		case VK_HOME:
			if(value > 0) {
				if(value > vt->col)
					vt->col = 0;
				else
					vt->col -= value;
			}
			res = true;
			break;
		case VK_END:
			if(value > 0) {
				if(vt->col + value > COLS - 1)
					vt->col = COLS - 1;
				else
					vt->col += value;
			}
			res = true;
			break;
		case VK_ESC_RESET:
			vt->foreground = WHITE;
			vt->background = BLACK;
			res = true;
			break;
		case VK_ESC_FG:
			vt->foreground = MIN(15,value);
			res = true;
			break;
		case VK_ESC_BG:
			vt->background = MIN(15,value);
			res = true;
			break;
		case VK_ESC_SET_ECHO:
			vt->echo = value ? true : false;
			res = true;
			break;
		case VK_ESC_SET_RL:
			vt->readLine = value ? true : false;
			if(vt->readLine) {
				/* reset reading */
				vt->rlBufPos = 0;
				vt->rlStartCol = vt->col;
			}
			res = true;
			break;
		case VK_ESC_KEYBOARD:
			*readKeyboard = value ? true : false;
			res = true;
			break;
		case VK_ESC_NAVI:
			vt->navigation = value ? true : false;
			break;
		case VK_ESC_BACKUP:
			if(!vt->screenBackup)
				vt->screenBackup = (char*)malloc(ROWS * COLS * 2);
			memcpy(vt->screenBackup,
					vt->buffer + vt->firstVisLine * COLS * 2,
					ROWS * COLS * 2);
			break;
		case VK_ESC_RESTORE:
			if(vt->screenBackup) {
				memcpy(vt->buffer + vt->firstVisLine * COLS * 2,
						vt->screenBackup,
						ROWS * COLS * 2);
				free(vt->screenBackup);
				vt->screenBackup = NULL;
				vterm_refreshScreen(vt);
			}
			break;
	}

	return res;
}

void vterm_handleKeycode(bool isBreak,u32 keycode) {
	sKeymapEntry *e;
	sVTerm *vt = activeVT;
	char c;

	if(vt == NULL)
		return;

	/* handle shift, alt and ctrl */
	switch(keycode) {
		case VK_LSHIFT:
		case VK_RSHIFT:
			shiftDown = !isBreak;
			break;
		case VK_LALT:
		case VK_RALT:
			altDown = !isBreak;
			break;
		case VK_LCTRL:
		case VK_RCTRL:
			ctrlDown = !isBreak;
			break;
	}

	/* we don't need breakcodes anymore */
	if(isBreak)
		return;

	e = keymaps[vt->keymap](keycode);
	if(e != NULL) {
		if(shiftDown)
			c = e->shift;
		else if(altDown)
			c = e->alt;
		else
			c = e->def;

		if(shiftDown && vt->navigation) {
			switch(keycode) {
				case VK_PGUP:
					vterm_scroll(vt,ROWS);
					return;
				case VK_PGDOWN:
					vterm_scroll(vt,-ROWS);
					return;
				case VK_UP:
					vterm_scroll(vt,1);
					return;
				case VK_DOWN:
					vterm_scroll(vt,-1);
					return;
			}
		}

		if(c == NPRINT || ctrlDown) {
			/* handle ^C, ^D and so on */
			if(ctrlDown) {
				switch(keycode) {
					case VK_C:
						sendSignal(SIG_INTRPT,activeVT->index);
						break;
					case VK_D:
						if(vt->readLine) {
							vterm_rlPutchar(vt,IO_EOF);
							vterm_rlFlushBuf(vt);
						}
						break;
					case VK_1 ... VK_9: {
						u32 index = keycode - VK_1;
						if(index < VTERM_COUNT && activeVT->index != index)
							vterm_selectVTerm(index);
					}
					return;
				}
			}

			/* in reading mode? */
			if(vt->readLine) {
				if(vt->echo)
					vterm_rlHandleKeycode(vt,keycode);
			}
			/* send escape-code */
			else {
				char escape[] = {'\033',keycode,(altDown << STATE_ALT) |
					(ctrlDown << STATE_CTRL) |
					(shiftDown << STATE_SHIFT)};
				if(rb_length(vt->inbuf) == 0)
					setDataReadable(vt->sid,true);
				rb_writen(vt->inbuf,escape,3);
			}
		}
		else {
			if(vt->readLine)
				vterm_rlPutchar(vt,c);
			else {
				if(rb_length(vt->inbuf) == 0)
					setDataReadable(vt->sid,true);
				rb_write(vt->inbuf,&c);
			}
		}

		if(vt->echo)
			vterm_setCursor(vt);
	}
}

static void vterm_rlFlushBuf(sVTerm *vt) {
	u32 i,bufPos = vterm_rlGetBufPos(vt);
	if(vt->echo)
		bufPos++;

	if(rb_length(vt->inbuf) == 0)
		setDataReadable(vt->sid,true);

	i = 0;
	while(bufPos > 0) {
		rb_write(vt->inbuf,vt->rlBuffer + i);
		bufPos--;
		i++;
	}
	vt->rlBufPos = 0;
}

static void vterm_rlPutchar(sVTerm *vt,char c) {
	switch(c) {
		case '\b': {
			u32 bufPos = vterm_rlGetBufPos(vt);
			if(bufPos > 0) {
				if(vt->echo) {
					u32 i = (vt->currLine * COLS * 2) + (vt->row * COLS * 2) + (vt->col * 2);
					/* move the characters back in the buffer */
					memmove(vt->buffer + i - 2,vt->buffer + i,(COLS - vt->col) * 2);
					vt->col--;
				}

				/* move chars back */
				memmove(vt->rlBuffer + bufPos - 1,vt->rlBuffer + bufPos,vt->rlBufPos - bufPos);
				/* remove last */
				vt->rlBuffer[vt->rlBufPos - 1] = '\0';
				vt->rlBufPos--;

				/* overwrite line */
				vterm_refreshLines(vt,vt->row,1);
			}
		}
		break;

		case '\r':
		case '\a':
		case '\t':
			/* ignore */
			break;

		default: {
			bool flushed = false;
			bool moved = false;
			u32 bufPos = vterm_rlGetBufPos(vt);

			/* increase buffer size? */
			if(vt->rlBuffer && bufPos >= vt->rlBufSize) {
				vt->rlBufSize += RLBUF_INCR;
				vt->rlBuffer = (char*)realloc(vt->rlBuffer,vt->rlBufSize);
			}
			if(vt->rlBuffer == NULL)
				return;

			/* move chars forward */
			if(bufPos < vt->rlBufPos) {
				memmove(vt->rlBuffer + bufPos + 1,vt->rlBuffer + bufPos,vt->rlBufPos - bufPos);
				moved = true;
			}

			/** add char */
			vt->rlBuffer[bufPos] = c;
			vt->rlBufPos++;

			/* TODO later we should allow "multiline-editing" */
			if(c == '\n' || vt->rlStartCol + vt->rlBufPos >= COLS) {
				flushed = true;
				vterm_rlFlushBuf(vt);
			}

			/* echo character, if required */
			if(vt->echo) {
				if(moved && !flushed) {
					u32 count = vt->rlBufPos - bufPos + 1;
					char *copy = (char*)malloc(count * sizeof(char));
					if(copy != NULL) {
						bool dummy;
						/* print the end of the buffer again */
						strncpy(copy,vt->rlBuffer + bufPos,count - 1);
						copy[count - 1] = '\0';
						vterm_puts(vt,copy,count - 1,false,&dummy);
						free(copy);

						/* reset cursor */
						vt->col = vt->rlStartCol + bufPos + 1;
					}
				}
				else if(c != IO_EOF) {
					u8 oldRow = vt->row,oldCol = vt->col;
					vterm_putchar(vt,c);
					if(vt->row != oldRow || vt->col != oldCol)
						vterm_sendChar(vt,oldRow,oldCol);
				}
			}
			if(flushed)
				vt->rlStartCol = vt->col;
		}
		break;
	}
}

static u32 vterm_rlGetBufPos(sVTerm *vt) {
	if(vt->echo)
		return vt->col - vt->rlStartCol;
	else
		return vt->rlBufPos;
}

static bool vterm_rlHandleKeycode(sVTerm *vt,u8 keycode) {
	bool res = false;
	switch(keycode) {
		case VK_LEFT:
			if(vt->col > vt->rlStartCol)
				vt->col--;
			res = true;
			break;
		case VK_RIGHT:
			if(vt->col < vt->rlStartCol + vt->rlBufPos)
				vt->col++;
			res = true;
			break;
		case VK_HOME:
			if(vt->col != vt->rlStartCol)
				vt->col = vt->rlStartCol;
			res = true;
			break;
		case VK_END:
			if(vt->col != vt->rlStartCol + vt->rlBufPos)
				vt->col = vt->rlStartCol + vt->rlBufPos;
			res = true;
			break;
	}
	return res;
}
