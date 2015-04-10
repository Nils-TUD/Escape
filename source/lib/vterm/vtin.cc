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

#include <esc/ipc/vtermdevice.h>
#include <sys/common.h>
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/keycodes.h>
#include <sys/time.h>
#include <vterm/vtctrl.h>
#include <vterm/vtin.h>
#include <vterm/vtout.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RLBUF_INCR			20

static void vtin_rlFlushBuf(sVTerm *vt);
static size_t vtin_rlGetBufPos(sVTerm *vt);
static bool vtin_rlHandleKeycode(sVTerm *vt,uchar keycode);

void vtin_handleKey(sVTerm *vt,uchar keycode,uchar modifier,char c) {
	if(!(modifier & STATE_BREAK)) {
		if((modifier & STATE_SHIFT) && vt->navigation) {
			switch(keycode) {
				case VK_HOME:
					vtctrl_scroll(vt,vt->rows * HISTORY_SIZE);
					return;
				case VK_END:
					vtctrl_scroll(vt,-vt->rows * HISTORY_SIZE);
					return;
				case VK_PGUP:
					vtctrl_scroll(vt,vt->rows);
					return;
				case VK_PGDOWN:
					vtctrl_scroll(vt,-vt->rows);
					return;
				case VK_UP:
					vtctrl_scroll(vt,1);
					return;
				case VK_DOWN:
					vtctrl_scroll(vt,-1);
					return;
			}
		}

		if(c == 0 || (modifier & (STATE_CTRL | STATE_ALT))) {
			if(modifier & STATE_CTRL) {
				switch(keycode) {
					case VK_C:
						/* send interrupt to shell */
						if(vt->shellPid) {
							if(kill(vt->shellPid,SIGINT) < 0)
								printe("Unable to send SIGINT to %d",vt->shellPid);
						}
						return;
					case VK_D:
						vt->inbufEOF = true;
						if(vt->readLine)
							vtin_rlFlushBuf(vt);
						return;
				}
			}

			/* in reading mode? */
			if(vt->readLine) {
				if(vt->echo)
					vtin_rlHandleKeycode(vt,keycode);
			}
		}
		if(c && vt->readLine)
			vtin_rlPutchar(vt,c);
	}

	/* send escape-code when we're not in readline-mode */
	if(!vt->readLine) {
		char escape[SSTRLEN("\033[kc;123;123;15]") + 1];
		/* we want to treat the character as unsigned here and extend it to 32bit */
		uint code = *(uchar*)&c;
		snprintf(escape,sizeof(escape),"\033[kc;%u;%u;%u]",code,keycode,modifier);
		vt->inbuf->writen(escape,strlen(escape));
	}
	if(!(modifier & STATE_BREAK)) {
		if(vt->echo && vt->setCursor)
			vt->setCursor(vt);
	}
}

void vtin_input(sVTerm *vt,const char *str,size_t len) {
	if(vt->readLine) {
		while(len-- > 0)
			vtin_rlPutchar(vt,*str++);
	}
	else {
		vtctrl_resizeInBuf(vt,vt->inbuf->length() + len);
		vt->inbuf->writen(str,len);
	}
}

static void vtin_selectWord(sVTerm *vt,int x,int y) {
	static const char *sepchars = " \t";
	vtin_removeSelection(vt);

	// stop if we're not in a word
	char *line = vt->lines[vt->firstVisLine + y];
	if(strchr(sepchars,line[x * 2]) != NULL)
		return;

	// find start and end of word
	int start = x;
	while(start >= 0 && strchr(sepchars,line[start * 2]) == NULL)
		start--;
	int end = x + 1;
	while((size_t)end < vt->cols && strchr(sepchars,line[end * 2]) == NULL)
		end++;

	// select the word
	vt->selStart = (vt->firstVisLine + y) * vt->cols + start + 1;
	vt->selEnd = (vt->firstVisLine + y) * vt->cols + end;
	vt->selDir = FORWARD;
	vtin_changeColorRange(vt,vt->selStart,vt->selEnd);
}

void vtin_handleMouse(sVTerm *vt,size_t x,size_t y,int z,bool select) {
	if(z != 0) {
		if(vt->navigation)
			vtctrl_scroll(vt,z);
		return;
	}

	// handle double clicks
	if(vt->mclicks == 0)
		vt->mlastClick = rdtsc();
	if(select) {
		if(~vt->mclicks & 1)
			vt->mclicks++;
		else
			vt->mclicks = 0;
	}
	else {
		if(vt->mclicks & 1) {
			vt->mclicks++;
			if(vt->mclicks == 4) {
				uint64_t timediff = tsctotime(rdtsc() - vt->mlastClick);
				if(timediff < DOUBLE_CLICK_TIME)
					vtin_selectWord(vt,x,y);
				vt->mclicks = 0;
			}
		}
		else if(vt->mclicks > 4 || vt->mcol != x || vt->mrow != y)
			vt->mclicks = 0;
	}

	if(select) {
		vtin_removeCursor(vt);

		// determine new start and end position
		size_t nstart = vt->selStart, nend = vt->selEnd;
		size_t pos = (vt->firstVisLine + y) * vt->cols + x;
		switch(vt->selDir) {
			case NONE:
				// if there is no direction, start a new selection
				nstart = pos;
				nend = pos;
				vt->selDir = FORWARD;
				break;

			case FORWARD:
				// switch direction?
				if(pos < vt->selStart) {
					nstart = pos;
					nend = vt->selStart;
					vt->selDir = BACKWARDS;
				}
				else
					nend = pos;
				break;

			case BACKWARDS:
				if(pos > vt->selEnd) {
					nstart = vt->selEnd;
					nend = pos;
					vt->selDir = FORWARD;
				}
				else
					nstart = pos;
				break;
		}

		// change selection if necessary
		if(nstart != vt->selStart || nend != vt->selEnd) {
			if(vt->selStart != vt->selEnd)
				vtin_changeColorRange(vt,vt->selStart,vt->selEnd);

			vt->selStart = nstart;
			vt->selEnd = nend;

			if(vt->selStart != vt->selEnd)
				vtin_changeColorRange(vt,vt->selStart,vt->selEnd);
		}
	}
	else if(vt->mcol != x || vt->mrow != y) {
		vtin_removeCursor(vt);

		vtin_changeColor(vt,x,vt->firstVisLine + y);
		vtctrl_markDirty(vt,x,vt->firstVisLine + y,1,1);

		vt->selDir = NONE;
		vt->mcol = x;
		vt->mrow = y;
		vt->mrowRel = vt->firstVisLine;
	}
}

void vtin_changeColor(sVTerm *vt,int x,int y) {
	char *line  = vt->lines[y];
	unsigned char old = line[x * 2 + 1];
	line[x * 2 + 1] = old >> 4 | ((old & 0xF) << 4);
}

void vtin_changeColorRange(sVTerm *vt,size_t start,size_t end) {
	// for simplicity, redraw entire lines
	int row = start / vt->cols;
	int rows = (end - start + vt->cols - 1) / vt->cols;
	vtctrl_markDirty(vt,0,row,vt->cols,rows);

	while(start != end) {
		vtin_changeColor(vt,start % vt->cols,start / vt->cols);
		start++;
	}
}

void vtin_removeCursor(sVTerm *vt) {
	if(vt->mcol != static_cast<size_t>(-1)) {
		vtin_changeColor(vt,vt->mcol,vt->mrow + vt->mrowRel);
		vtctrl_markDirty(vt,vt->mcol,vt->mrow + vt->mrowRel,1,1);
		vt->mcol = -1;
	}
}

void vtin_removeSelection(sVTerm *vt) {
	if(vt->selStart != vt->selEnd) {
		vtin_changeColorRange(vt,vt->selStart,vt->selEnd);
		vt->selStart = vt->selEnd = 0;
		vt->selDir = NONE;
	}
}

bool vtin_hasData(sVTerm *vt) {
	return vt->inbufEOF || vt->inbuf->length() > 0;
}

size_t vtin_gets(sVTerm *vt,char *buffer,size_t count) {
	size_t res = 0;
	if(buffer)
		res = vt->inbuf->readn(buffer,count);
	if(vt->inbuf->length() == 0)
		vt->inbufEOF = false;
	return res;
}

void vtin_rlPutchar(sVTerm *vt,char c) {
	switch(c) {
		case '\b': {
			size_t bufPos = vtin_rlGetBufPos(vt);
			if(bufPos > 0) {
				if(vt->echo) {
					/* move the characters back in the buffer */
					char *line = vt->lines[vt->currLine + vt->row];
					memmove(line + (vt->col - 1) * 2,line + vt->col * 2,(vt->cols - vt->col) * 2);
					vt->col--;
				}

				/* move chars back */
				memmove(vt->rlBuffer + bufPos - 1,vt->rlBuffer + bufPos,vt->rlBufPos - bufPos);
				/* remove last */
				vt->rlBuffer[vt->rlBufPos - 1] = '\0';
				vt->rlBufPos--;

				/* overwrite line */
				/* TODO just refresh the required part */
				vtctrl_markDirty(vt,0,vt->firstVisLine + vt->row,vt->cols,1);
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
			size_t bufPos = vtin_rlGetBufPos(vt);

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

			/* add char (newlines always at the end) */
			if(c == '\n')
				vt->rlBuffer[vt->rlBufPos] = c;
			else
				vt->rlBuffer[bufPos] = c;
			vt->rlBufPos++;

			/* TODO later we should allow "multiline-editing" */
			if(c == '\n' || vt->rlStartCol + vt->rlBufPos >= vt->cols) {
				flushed = true;
				vtin_rlFlushBuf(vt);
			}

			/* echo character, if required */
			if(vt->echo) {
				if(moved && !flushed) {
					size_t i,count = vt->rlBufPos - bufPos;
					for(i = 0; i < count; i++)
						vtout_putchar(vt,vt->rlBuffer[bufPos + i]);
					/* reset cursor */
					vt->col = vt->rlStartCol + bufPos + 1;
				}
				else if(c != EOF)
					vtout_putchar(vt,c);
			}
			if(flushed)
				vt->rlStartCol = vt->col;
		}
		break;
	}
}

static void vtin_rlFlushBuf(sVTerm *vt) {
	for(size_t i = 0; vt->rlBufPos > 0; ++i) {
		vt->inbuf->write(vt->rlBuffer[i]);
		vt->rlBufPos--;
	}
}

static size_t vtin_rlGetBufPos(sVTerm *vt) {
	if(vt->echo)
		return vt->col - vt->rlStartCol;
	else
		return vt->rlBufPos;
}

static bool vtin_rlHandleKeycode(sVTerm *vt,uchar keycode) {
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
