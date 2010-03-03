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
#include <esc/keycodes.h>
#include <esc/io.h>
#include <esc/ports.h>
#include <esc/heap.h>
#include <messages.h>
#include <esccodes.h>
#include <string.h>
#include "vtout.h"
#include "vtin.h"
#include "vtctrl.h"

/**
 * Inserts a new line
 *
 * @param vt the vterm
 */
static void vterm_newLine(sVTerm *vt);

/**
 * Deletes <count> in front of the current position, if possible
 *
 * @param vt the vterm
 * @param count the number of characters
 */
static void vterm_delete(sVTerm *vt,u32 count);

/**
 * Handles an escape-code
 *
 * @param vt the vterm
 * @param str the string
 * @return true if something has been done
 */
static bool vterm_handleEscape(sVTerm *vt,char **str);

static sMsg msg;

void vterm_puts(sVTerm *vt,char *str,u32 len,bool resetRead) {
	char c,*start = str;

	/* are we waiting to finish an escape-code? */
	if(vt->escapePos >= 0) {
		u32 oldLen = vt->escapePos;
		char *escPtr = vt->escapeBuf;
		u16 length = MIN((s32)len,MAX_ESCC_LENGTH - vt->escapePos - 1);
		/* append the string */
		memcpy(vt->escapeBuf + vt->escapePos,str,length);
		vt->escapePos += length;
		vt->escapeBuf[vt->escapePos] = '\0';

		/* try it again */
		if(!vterm_handleEscape(vt,&escPtr)) {
			/* if no space is left, quit and simply print the code */
			if(vt->escapePos >= MAX_ESCC_LENGTH - 1) {
				u32 i;
				for(i = 0; i < MAX_ESCC_LENGTH; i++) {
					if(vt->printToRL)
						vterm_rlPutchar(vt,vt->escapeBuf[i]);
					else {
						vterm_putchar(vt,vt->escapeBuf[i]);
						if(resetRead) {
							vt->rlBufPos = 0;
							vt->rlStartCol = vt->col;
						}
					}
				}
			}
			/* otherwise try again next time */
			else
				return;
		}
		/* skip escape-code */
		str += (escPtr - vt->escapeBuf) - oldLen;
		vt->escapePos = -1;
	}

	while((c = *str)) {
		if(c == '\033') {
			str++;
			/* if the escape-code is incomplete, store what we have so far and wait for
			 * further input */
			if(!vterm_handleEscape(vt,&str)) {
				u32 count = MIN(MAX_ESCC_LENGTH,len - (str - start));
				memcpy(vt->escapeBuf,str,count);
				vt->escapePos = count;
				break;
			}
			continue;
		}
		if(vt->printToRL)
			vterm_rlPutchar(vt,c);
		else {
			vterm_putchar(vt,c);
			if(resetRead) {
				vt->rlBufPos = 0;
				vt->rlStartCol = vt->col;
			}
		}
		str++;
	}

	/* scroll to current line, if necessary */
	if(vt->firstVisLine != vt->currLine)
		vterm_scroll(vt,vt->firstVisLine - vt->currLine);
}

void vterm_putchar(sVTerm *vt,char c) {
	u32 i;

	/* move all one line up, if necessary */
	if(vt->row >= vt->rows) {
		vterm_newLine(vt);
		vt->row--;
	}

#ifdef LOGSERIAL
	/* write to bochs(0xe9) / qemu(0x3f8) console */
	/* a few characters don't make much sense here */
	if(c != '\r' && c != '\a' && c != '\b' && c != '\t') {
		while((inByte(0x3f8 + 5) & 0x20) == 0);
		outByte(0x3f8,c);
	}
#endif

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
			msg.args.arg2 = 60;
			send(vt->speaker,MSG_SPEAKER_BEEP,&msg,sizeof(msg.args));
			break;

		case '\b':
			vterm_delete(vt,1);
			break;

		case '\t':
			i = TAB_WIDTH - vt->col % TAB_WIDTH;
			while(i-- > 0) {
				vterm_putchar(vt,' ');
			}
			break;

		default: {
			/* do an explicit newline if necessary */
			if(vt->col >= vt->cols)
				vterm_putchar(vt,'\n');

			i = (vt->currLine * vt->cols * 2) + (vt->row * vt->cols * 2) + (vt->col * 2);

			/* write to buffer */
			vt->buffer[i] = c;
			vt->buffer[i + 1] = (vt->background << 4) | vt->foreground;

			vterm_markDirty(vt,vt->row * vt->cols * 2 + vt->col * 2,2);
			vt->col++;
		}
		break;
	}
}

static void vterm_newLine(sVTerm *vt) {
	char *src,*dst;
	u32 i,count = (HISTORY_SIZE * vt->rows - vt->firstLine) * vt->cols * 2;
	/* move one line back */
	if(vt->firstLine > 0) {
		dst = vt->buffer + ((vt->firstLine - 1) * vt->cols * 2);
		vt->firstLine--;
	}
	/* overwrite first line */
	else
		dst = vt->buffer + (vt->firstLine * vt->cols * 2);
	src = dst + vt->cols * 2;
	memmove(dst,src,count);

	/* clear last line */
	dst = vt->buffer + (vt->currLine + vt->row - 1) * vt->cols * 2;
	for(i = 0; i < vt->cols * 2; i += 4) {
		*dst++ = 0x20;
		*dst++ = (vt->defBackground << 4) | vt->defForeground;
		*dst++ = 0x20;
		*dst++ = (vt->defBackground << 4) | vt->defForeground;
	}

	/* we've scrolled one line up */
	vt->upScroll++;
}

static void vterm_delete(sVTerm *vt,u32 count) {
	if((!vt->readLine && vt->col >= count) || (vt->readLine && vt->rlBufPos >= count)) {
		if(!vt->readLine || vt->echo) {
			u32 i = (vt->currLine * vt->cols * 2) + (vt->row * vt->cols * 2) + (vt->col * 2);
			/* move the characters back in the buffer */
			memmove(vt->buffer + i - 2 * count,vt->buffer + i,(vt->cols - vt->col) * 2);
			vt->col -= count;
		}

		if(vt->readLine) {
			vt->rlBuffer[vt->rlBufPos] = '\0';
			vt->rlBufPos -= count;
		}

		/* overwrite line */
		/* TODO just refresh the required part */
		vterm_markDirty(vt,vt->row * vt->cols * 2 + vt->col * 2,vt->cols * 2);
	}
	else {
		/* beep */
		msg.args.arg1 = 1000;
		msg.args.arg2 = 60;
		send(vt->speaker,MSG_SPEAKER_BEEP,&msg,sizeof(msg.args));
	}
}

static bool vterm_handleEscape(sVTerm *vt,char **str) {
	s32 cmd,n1,n2,n3;
	cmd = escc_get((const char**)str,&n1,&n2,&n3);
	if(cmd == ESCC_INCOMPLETE)
		return false;

	switch(cmd) {
		case ESCC_MOVE_LEFT:
			vt->col = MAX(0,vt->col - n1);
			break;
		case ESCC_MOVE_RIGHT:
			vt->col = MIN(vt->cols - 1,vt->col + n1);
			break;
		case ESCC_MOVE_UP:
			vt->row = MAX(1,vt->row - n1);
			break;
		case ESCC_MOVE_DOWN:
			vt->row = MIN(vt->rows - 1,vt->row + n1);
			break;
		case ESCC_MOVE_HOME:
			vt->col = 0;
			vt->row = 1;
			break;
		case ESCC_MOVE_LINESTART:
			vt->col = 0;
			break;
		case ESCC_SIM_INPUT:
			vt->printToRL = n1 == 1;
			break;
		case ESCC_DEL_FRONT:
			vterm_delete(vt,n1);
			break;
		case ESCC_DEL_BACK:
			if(vt->readLine) {
				vt->rlBufPos = MIN(vt->rlBufSize - 1,vt->rlBufPos + n1);
				vt->rlBufPos = MIN((u32)vt->cols - vt->rlStartCol,vt->rlBufPos);
				vt->col = vt->rlBufPos + vt->rlStartCol;
			}
			else
				vt->col = MIN(vt->cols - 1,vt->col + n1);
			break;
		case ESCC_GOTO_XY:
			vt->col = MAX(0,MIN(vt->cols - 1,n1));
			vt->row = MAX(0,MIN(vt->rows - 2,n2)) + 1;
			break;
		case ESCC_COLOR:
			if(n1 != ESCC_ARG_UNUSED)
				vt->foreground = MIN(15,n1);
			else
				vt->foreground = vt->defForeground;
			if(n2 != ESCC_ARG_UNUSED)
				vt->background = MIN(15,n2);
			else
				vt->background = vt->defBackground;
			break;
	}
	return true;
}
