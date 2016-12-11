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

#include <esc/proto/vterm.h>
#include <sys/common.h>
#include <sys/conf.h>
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/keycodes.h>
#include <sys/messages.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <vterm/vtctrl.h>
#include <vterm/vtin.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* the number of chars to keep in history */
#define INITIAL_RLBUF_SIZE	50

static char *vtctrl_createEmptyLine(sVTerm *vt,size_t cols);
static char **vtctrl_createLines(size_t cols,size_t rows);
static void vtctrl_freeLines(char **lines,size_t rows);

bool vtctrl_init(sVTerm *vt,esc::Screen::Mode *mode) {
	/* init state */
	vt->mutex = new std::mutex();
	/* by default we have no handlers for that */
	vt->setCursor = NULL;
	vt->cols = mode->cols;
	vt->rows = mode->rows;
	vt->col = 0;
	vt->row = vt->rows - 1;
	vt->lastCol = 0;
	vt->lastRow = 0;
	vt->mcol = -1;
	vt->mrow = 0;
	vt->mrowRel = 0;
	vt->mclicks = 0;
	vt->mlastClick = 0;
	vt->selStart = 0;
	vt->selEnd = 0;
	vt->selDir = NONE;
	vt->upCol = vt->cols;
	vt->upRow = vt->rows;
	vt->upWidth = 0;
	vt->upHeight = 0;
	vt->upScroll = 0;
	vt->foreground = vt->defForeground;
	vt->background = vt->defBackground;
	/* start on first line of the last page */
	vt->firstLine = HISTORY_SIZE * vt->rows - vt->rows;
	vt->currLine = HISTORY_SIZE * vt->rows - vt->rows;
	vt->firstVisLine = HISTORY_SIZE * vt->rows - vt->rows;
	/* default behaviour */
	vt->echo = true;
	vt->readLine = true;
	vt->navigation = true;
	vt->printToRL = false;
	vt->printToCom1 = sysconf(CONF_LOG);
	vt->escapePos = -1;
	vt->rlStartCol = 0;
	vt->shellPid = 0;
	vt->screenBackup = NULL;
	vt->lines = vtctrl_createLines(vt->cols,vt->rows);
	if(!vt->lines) {
		printe("Unable to allocate mem for vterm-buffer");
		return false;
	}
	vt->rlBufSize = INITIAL_RLBUF_SIZE;
	vt->rlBufPos = 0;
	vt->rlBuffer = (char*)malloc(vt->rlBufSize * sizeof(char));
	if(vt->rlBuffer == NULL) {
		printe("Unable to allocate memory for readline-buffer");
		return false;
	}

	vt->inbufEOF = false;
	vt->inbufSize = INPUT_BUF_SIZE;
	vt->inbuf = new esc::RingBuffer<char>(INPUT_BUF_SIZE,esc::RB_OVERWRITE);

	/* create and init empty line */
	vt->emptyLine = vtctrl_createEmptyLine(vt,vt->cols);
	if(!vt->emptyLine) {
		printe("Unable to allocate memory for empty line");
		return false;
	}

	/* fill buffer with spaces to ensure that the cursor is visible (spaces, white on black) */
	for(size_t i = 0; i < vt->rows * HISTORY_SIZE; ++i)
		memcpy(vt->lines[i],vt->emptyLine,vt->cols * 2);
	return true;
}

void vtctrl_destroy(sVTerm *vt) {
	delete vt->mutex;
	delete vt->inbuf;
	vtctrl_freeLines(vt->lines,vt->rows);
	free(vt->emptyLine);
	free(vt->rlBuffer);
}

bool vtctrl_resize(sVTerm *vt,size_t cols,size_t rows) {
	bool res = false;
	if(vt->cols != cols || vt->rows != rows) {
		size_t c,r,oldr,color;
		size_t ccols = esc::Util::min(cols,vt->cols);
		char *buf,*oldBuf,**old = vt->lines,*oldempty = vt->emptyLine;
		vt->lines = vtctrl_createLines(cols,rows);
		if(!vt->lines) {
			vt->lines = old;
			return false;
		}

		vt->emptyLine = vtctrl_createEmptyLine(vt,cols);
		if(!vt->emptyLine) {
			vt->lines = old;
			vt->emptyLine = oldempty;
			return false;
		}

		color = (vt->background << 4) | vt->foreground;
		r = 0;
		if(rows > vt->rows) {
			size_t limit = (rows - vt->rows) * HISTORY_SIZE;
			for(; r < limit; r++) {
				buf = vt->lines[r];
				for(c = 0; c < cols; c++) {
					*buf++ = ' ';
					*buf++ = color;
				}
			}
			oldr = 0;
		}
		else
			oldr = (vt->rows - rows) * HISTORY_SIZE;
		for(; r < rows * HISTORY_SIZE; r++, oldr++) {
			buf = vt->lines[r];
			oldBuf = old[oldr];
			memcpy(buf,oldBuf,ccols * 2);
			buf += ccols * 2;
			for(c = ccols; c < cols; c++) {
				*buf++ = ' ';
				*buf++ = color;
			}
		}

		if(vt->rows * HISTORY_SIZE - vt->firstLine >= rows * HISTORY_SIZE)
			vt->firstLine = 0;
		else
			vt->firstLine = rows * HISTORY_SIZE - (vt->rows * HISTORY_SIZE - vt->firstLine);
		vt->firstLine += vt->rows - rows;
		if(vt->rows * HISTORY_SIZE - vt->currLine >= rows * HISTORY_SIZE)
			vt->currLine = 0;
		else
			vt->currLine = rows * HISTORY_SIZE - (vt->rows * HISTORY_SIZE - vt->currLine);
		vt->currLine += vt->rows - rows;
		if(vt->rows * HISTORY_SIZE - vt->firstVisLine >= rows * HISTORY_SIZE)
			vt->firstVisLine = 0;
		else
			vt->firstVisLine = rows * HISTORY_SIZE - (vt->rows * HISTORY_SIZE - vt->firstVisLine);
		vt->firstVisLine += vt->rows - rows;

		/* TODO update screenbackup */
		vtctrl_freeLines(old,vt->rows);
		free(oldempty);
		vt->col = esc::Util::min(vt->col,cols - 1);
		vt->row = esc::Util::min(rows - 1,rows - (vt->rows - vt->row));
		vt->cols = cols;
		vt->rows = rows;
		vt->upCol = vt->cols;
		vt->upRow = vt->rows;
		vt->upWidth = 0;
		vt->upHeight = 0;
		vtin_removeCursor(vt);
		vtin_removeSelection(vt);
		vtctrl_markScrDirty(vt);
		res = true;
	}
	return res;
}

int vtctrl_control(sVTerm *vt,uint cmd,int arg1,int arg2) {
	int res = 0;
	switch(cmd) {
		case MSG_VT_SHELLPID:
			vt->shellPid = arg1;
			break;
		case MSG_VT_GETFLAG: {
			switch(arg1) {
				case esc::VTerm::FL_ECHO:
					res = vt->echo;
					break;
				case esc::VTerm::FL_NAVI:
					res = vt->navigation;
					break;
				case esc::VTerm::FL_READLINE:
					res = vt->readLine;
					break;
				default:
					res = -EINVAL;
					break;
			}
		}
		break;
		case MSG_VT_SETFLAG: {
			switch(arg1) {
				case esc::VTerm::FL_ECHO:
					vt->echo = arg2;
					break;
				case esc::VTerm::FL_NAVI:
					vt->navigation = arg2;
					break;
				case esc::VTerm::FL_READLINE:
					vt->readLine = arg2;
					if(arg2) {
						/* reset reading */
						vt->rlBufPos = 0;
						vt->rlStartCol = vt->col;
					}
					break;
				default:
					res = -EINVAL;
					break;
			}
		}
		break;
		case MSG_VT_BACKUP:
			if(!vt->screenBackup)
				vt->screenBackup = (char*)malloc(vt->rows * vt->cols * 2);
			if(vt->screenBackup) {
				for(size_t r = 0; r < vt->rows; ++r) {
					memcpy(vt->screenBackup + r * vt->cols * 2,
						vt->lines[vt->firstVisLine + r],vt->cols * 2);
				}
			}
			else
				res = -ENOMEM;
			vt->backupCol = vt->col;
			vt->backupRow = vt->row;
			break;
		case MSG_VT_RESTORE:
			if(vt->screenBackup) {
				for(size_t r = 0; r < vt->rows; ++r) {
					memcpy(vt->lines[vt->firstVisLine + r],
						vt->screenBackup + r * vt->cols * 2,vt->cols * 2);
				}
				free(vt->screenBackup);
				vt->screenBackup = NULL;
				vt->col = vt->backupCol;
				vt->row = vt->backupRow;
				vtctrl_markScrDirty(vt);
			}
			break;
		case MSG_VT_ISVTERM:
			res = 1;
			break;
	}
	return res;
}

void vtctrl_scroll(sVTerm *vt,int lines) {
	size_t old;
	old = vt->firstVisLine;
	if(lines > 0) {
		/* ensure that we don't scroll above the first line with content */
		vt->firstVisLine = esc::Util::max((int)vt->firstLine,(int)vt->firstVisLine - lines);
	}
	else {
		/* ensure that we don't scroll behind the last line */
		vt->firstVisLine = esc::Util::min(HISTORY_SIZE * vt->rows - vt->rows,vt->firstVisLine - lines);
	}

	if(old != vt->firstVisLine)
		vt->upScroll -= lines;
}

void vtctrl_resizeInBuf(sVTerm *vt,size_t length) {
	if(vt->inbufSize < length) {
		esc::RingBuffer<char> *nbuf = new esc::RingBuffer<char>(length);
		if(!nbuf)
			return;
		nbuf->move(*vt->inbuf,vt->inbuf->length());
		vt->inbuf = nbuf;
		vt->inbufSize = length;
	}
}

void vtctrl_getSelection(sVTerm *vt,esc::OStream &os) {
	if(vt->selStart != vt->selEnd) {
		size_t y = vt->selStart / vt->cols;
		size_t x = vt->selStart % vt->cols;
		for(size_t i = vt->selStart; i < vt->selEnd; ++i) {
			os.write(vt->lines[y][x * 2]);
			if(++x == vt->cols) {
				os.write('\n');
				y++;
				x = 0;
			}
		}
	}
}

void vtctrl_markScrDirty(sVTerm *vt) {
	vtctrl_markDirty(vt,0,vt->firstVisLine,vt->cols,vt->rows);
}

void vtctrl_markDirty(sVTerm *vt,uint col,uint row,size_t width,size_t height) {
	if(row >= vt->firstVisLine + vt->rows)
		return;
	if(row + height > vt->firstVisLine + vt->rows)
		height = vt->firstVisLine + vt->rows - row;
	else if(row < vt->firstVisLine) {
		if(height <= vt->firstVisLine - row)
			return;
		height -= vt->firstVisLine - row;
		row = vt->firstVisLine;
	}
	row -= vt->firstVisLine;

	int x = vt->upCol, y = vt->upRow;
	vt->upCol = esc::Util::min(vt->upCol,col);
	vt->upRow = esc::Util::min(vt->upRow,row);
	vt->upWidth = esc::Util::max(x + vt->upWidth,col + width) - vt->upCol;
	vt->upHeight = esc::Util::max(y + vt->upHeight,row + height) - vt->upRow;
	assert(vt->upWidth <= vt->cols);
	assert(vt->upHeight <= vt->rows);
}

static char *vtctrl_createEmptyLine(sVTerm *vt,size_t cols) {
	/* create and init empty line */
	char *emptyLine = (char*)malloc(cols * 2);
	if(!emptyLine)
		return NULL;
	uchar color = (vt->background << 4) | vt->foreground;
	for(size_t j = 0; j < cols; ++j) {
		emptyLine[j * 2] = ' ';
		emptyLine[j * 2 + 1] = color;
	}
	return emptyLine;
}

static char **vtctrl_createLines(size_t cols,size_t rows) {
	char **res = (char**)malloc(rows * HISTORY_SIZE * sizeof(char*));
	if(res == NULL)
		return NULL;
	for(size_t i = 0; i < rows * HISTORY_SIZE; ++i) {
		res[i] = (char*)malloc(cols * 2);
		if(res[i] == NULL)
			return NULL;
	}
	return res;
}

static void vtctrl_freeLines(char **lines,size_t rows) {
	for(size_t i = 0; i < rows * HISTORY_SIZE; ++i)
		free(lines[i]);
	free(lines);
}
