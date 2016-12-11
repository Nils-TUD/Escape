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

#include <sys/common.h>
#include <sys/esccodes.h>
#include <sys/io.h>
#include <sys/keycodes.h>
#include <sys/log.h>
#include <sys/messages.h>
#include <vterm/vtctrl.h>
#include <vterm/vtin.h>
#include <vterm/vtout.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void vtout_doPutchar(sVTerm *vt,char c,bool markDirty);
static void vtout_newLine(sVTerm *vt);
static void vtout_delete(sVTerm *vt,size_t count);
static void vtout_beep(sVTerm *vt);
static bool vtout_handleEscape(sVTerm *vt,char **str);

void vtout_puts(sVTerm *vt,char *str,size_t len,bool resetRead) {
	size_t oldRow;
	char c,*start = str;

	/* are we waiting to finish an escape-code? */
	if(vt->escapePos != (size_t)-1) {
		size_t oldLen = vt->escapePos;
		char *escPtr = vt->escapeBuf;
		size_t length = esc::Util::min(len,MAX_ESCC_LENGTH - vt->escapePos - 1);
		/* append the string */
		memcpy(vt->escapeBuf + vt->escapePos,str,length);
		vt->escapePos += length;
		vt->escapeBuf[vt->escapePos] = '\0';

		/* try it again */
		if(!vtout_handleEscape(vt,&escPtr)) {
			/* if no space is left, quit and simply print the code */
			if(vt->escapePos + 1 >= MAX_ESCC_LENGTH) {
				size_t i;
				for(i = 0; i < MAX_ESCC_LENGTH; i++) {
					if(vt->printToRL)
						vtin_rlPutchar(vt,vt->escapeBuf[i]);
					else {
						vtout_putchar(vt,vt->escapeBuf[i]);
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

	oldRow = vt->row;
	while((c = *str)) {
		if(c == '\033') {
			str++;
			/* if the escape-code is incomplete, store what we have so far and wait for
			 * further input */
			size_t oldEscRow = vt->row;
			if(!vtout_handleEscape(vt,&str)) {
				size_t count = esc::Util::min((size_t)MAX_ESCC_LENGTH,len - (str - start));
				memcpy(vt->escapeBuf,str,count);
				vt->escapePos = count;
				break;
			}

			/* if that changed the position, mark the so far updated part dirty */
			if(oldEscRow != vt->row) {
				vtctrl_markDirty(vt,0,vt->firstVisLine + oldRow,vt->cols,oldEscRow - oldRow + 1);
				oldRow = vt->row;
			}
			continue;
		}
		if(vt->printToRL)
			vtin_rlPutchar(vt,c);
		else {
			vtout_doPutchar(vt,c,false);
			if(resetRead) {
				vt->rlBufPos = 0;
				vt->rlStartCol = vt->col;
			}
		}
		str++;
	}

	/* mark dirty */
	vtctrl_markDirty(vt,0,vt->firstVisLine + oldRow,vt->cols,vt->row - oldRow + 1);
	/* scroll to current line, if necessary */
	if(vt->firstVisLine != vt->currLine)
		vtctrl_scroll(vt,vt->firstVisLine - vt->currLine);
}

void vtout_putchar(sVTerm *vt,char c) {
	vtout_doPutchar(vt,c,true);
}

static void vtout_doPutchar(sVTerm *vt,char c,bool markDirty) {
	/* move all one line up, if necessary */
	if(vt->row >= vt->rows) {
		vtout_newLine(vt);
		vt->row--;
	}

	if(vt->printToCom1 && c != '\r' && c != '\a' && c != '\t') {
		if(c == '\n')
			logc('\r');
		logc(c);
	}

	switch(c) {
		case '\n':
			/* to next line */
			vt->row++;
			/* move cursor to line start */
			vtout_putchar(vt,'\r');
			break;

		case '\r':
			/* to line-start */
			vt->col = 0;
			break;

		case '\a':
			vtout_beep(vt);
			break;

		case '\b':
			vtout_delete(vt,1);
			break;

		case '\t': {
			size_t i = TAB_WIDTH - vt->col % TAB_WIDTH;
			while(i-- > 0)
				vtout_putchar(vt,' ');
		}
		break;

		default: {
			/* do an explicit newline if necessary */
			if(vt->col >= vt->cols)
				vtout_putchar(vt,'\n');

			/* write to buffer */
			assert(vt->currLine + vt->row < HISTORY_SIZE * vt->rows);
			char *line  = vt->lines[vt->currLine + vt->row];
			line[vt->col * 2] = c;
			line[vt->col * 2 + 1] = (vt->background << 4) | vt->foreground;

			if(markDirty)
				vtctrl_markDirty(vt,vt->col,vt->firstVisLine + vt->row,1,1);
			vt->col++;
		}
		break;
	}
}

static void vtout_newLine(sVTerm *vt) {
	char **dst;
	size_t count = (HISTORY_SIZE * vt->rows - vt->firstLine) * sizeof(char*);

	/* hide cursor and selection */
	vtin_removeCursor(vt);
	vtin_removeSelection(vt);

	/* move one line back */
	if(vt->firstLine > 0)
		vt->firstLine--;
	dst = vt->lines + vt->firstLine;
	char *first = dst[0];
	memmove(dst,dst + 1,count);

	/* use first line for last one and clear it */
	char **line = vt->lines + vt->currLine + vt->row - 1;
	*line = first;
	memcpy(*line,vt->emptyLine,vt->cols * 2);

	/* we've scrolled one line up */
	vt->upScroll++;
}

static void vtout_delete(sVTerm *vt,size_t count) {
	if((!vt->readLine && vt->col >= count) || (vt->readLine && vt->rlBufPos >= count)) {
		if(!vt->readLine || vt->echo) {
			/* move the characters back in the buffer */
			char *line = vt->lines[vt->currLine + vt->row];
			memmove(line + (vt->col - count) * 2,line + vt->col * 2,(vt->cols - vt->col) * 2);
			vt->col -= count;
		}

		if(vt->readLine) {
			vt->rlBuffer[vt->rlBufPos] = '\0';
			vt->rlBufPos -= count;
		}

		/* overwrite line */
		/* TODO just refresh the required part */
		vtctrl_markDirty(vt,0,vt->firstVisLine + vt->row,vt->cols,1);
	}
	else
		vtout_beep(vt);
}

static void vtout_beep(sVTerm *vt) {
	if(vt->speaker)
		vt->speaker->beep(1000,60);
}

static bool vtout_handleEscape(sVTerm *vt,char **str) {
	int cmd,n1,n2,n3;
	cmd = escc_get((const char**)str,&n1,&n2,&n3);
	if(cmd == ESCC_INCOMPLETE)
		return false;

	switch(cmd) {
		case ESCC_MOVE_LEFT:
			vt->col = esc::Util::max(0,esc::Util::min((int)vt->cols - 1,(int)vt->col - n1));
			break;
		case ESCC_MOVE_RIGHT:
			vt->col = esc::Util::max(0,esc::Util::min((int)vt->cols - 1,(int)vt->col + n1));
			break;
		case ESCC_MOVE_UP:
			vt->row = esc::Util::max(0,esc::Util::min((int)vt->rows - 1,(int)vt->row - n1));
			break;
		case ESCC_MOVE_DOWN:
			vt->row = esc::Util::max(0,esc::Util::min((int)vt->rows - 1,(int)vt->row + n1));
			break;
		case ESCC_MOVE_HOME:
			vt->col = 0;
			vt->row = 0;
			break;
		case ESCC_MOVE_LINESTART:
			vt->col = 0;
			break;
		case ESCC_SIM_INPUT:
			vt->printToRL = n1 == 1;
			break;
		case ESCC_DEL_FRONT:
			vtout_delete(vt,n1);
			break;
		case ESCC_DEL_BACK:
			if(vt->readLine) {
				vt->rlBufPos = esc::Util::min(vt->rlBufSize - 1,vt->rlBufPos + n1);
				vt->rlBufPos = esc::Util::min((size_t)vt->cols - vt->rlStartCol,vt->rlBufPos);
				vt->col = vt->rlBufPos + vt->rlStartCol;
			}
			else
				vt->col = esc::Util::min(vt->cols - 1,vt->col + n1);
			break;
		case ESCC_GOTO_XY:
			vt->col = esc::Util::min(vt->cols - 1,(size_t)n1);
			vt->row = esc::Util::min(vt->rows - 1,(size_t)n2);
			break;
		case ESCC_COLOR:
			if(n1 != ESCC_ARG_UNUSED)
				vt->foreground = esc::Util::min(15,n1);
			else
				vt->foreground = vt->defForeground;
			if(n2 != ESCC_ARG_UNUSED)
				vt->background = esc::Util::min(15,n2);
			else
				vt->background = vt->defBackground;
			break;
	}
	return true;
}
