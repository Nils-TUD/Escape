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

#include <esc/common.h>
#include <esc/keycodes.h>
#include <esc/io.h>
#include <esc/driver.h>
#include <ipc/vtermdevice.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vterm/vtctrl.h>
#include <vterm/vtin.h>
#include <vterm/vtout.h>

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
		rb_writen(vt->inbuf,escape,strlen(escape));
	}
	if(!(modifier & STATE_BREAK)) {
		if(vt->echo && vt->setCursor)
			vt->setCursor(vt);
	}
}

bool vtin_hasData(sVTerm *vt) {
	return vt->inbufEOF || rb_length(vt->inbuf) > 0;
}

size_t vtin_gets(sVTerm *vt,char *buffer,size_t count) {
	size_t res = 0;
	if(buffer)
		res = rb_readn(vt->inbuf,buffer,count);
	if(rb_length(vt->inbuf) == 0)
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
				vtctrl_markDirty(vt,0,vt->row,vt->cols,1);
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
		rb_write(vt->inbuf,vt->rlBuffer + i);
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
