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

#include <esc/util.h>
#include <dbg/kb.h>
#include <sys/esccodes.h>
#include <assert.h>
#include <common.h>
#include <config.h>
#include <log.h>
#include <video.h>

Video Video::inst;

void Video::backup(char *buffer,ushort *r,ushort *c) const {
	LockGuard<SpinLock> g(&lock);
	copyScrToMem(buffer,screen(),VID_ROWS);
	*r = row;
	*c = col;
}

void Video::restore(const char *buffer,ushort r,ushort c) {
	LockGuard<SpinLock> g(&lock);
	copyMemToScr(screen(),buffer,VID_ROWS);
	row = r;
	col = c;
}

void Video::writec(char c) {
	if(c == '\0')
		return;

	size_t i;
	/* do an explicit newline if necessary */
	if(col >= VID_COLS) {
		if(Config::get(Config::LINE_BY_LINE))
			Keyboard::get(NULL,Keyboard::EVENT_PRESS,true);
		row++;
		col = 0;
	}
	move();

	if(c == '\n') {
		if(Config::get(Config::LINE_BY_LINE))
			Keyboard::get(NULL,Keyboard::EVENT_PRESS,true);
		row++;
		col = 0;
	}
	else if(c == '\r')
		col = 0;
	else if(c == '\t') {
		i = TAB_WIDTH - col % TAB_WIDTH;
		while(i-- > 0)
			writec(' ');
	}
	else {
		drawChar(col,row,c);
		col++;
	}
}

uchar Video::pipepad() const {
	return VID_COLS - col;
}

bool Video::escape(const char **str) {
	int n1,n2,n3;
	int cmd = escc_get(str,&n1,&n2,&n3);
	switch(cmd) {
		case ESCC_COLOR: {
			uchar fg = n1 == ESCC_ARG_UNUSED ? WHITE : esc::Util::min(9,n1);
			uchar bg = n2 == ESCC_ARG_UNUSED ? BLACK : esc::Util::min(9,n2);
			color = (bg << 4) | fg;
		}
		break;
		default:
			vassert(false,"Invalid escape-code ^[%d;%d,%d,%d]",cmd,n1,n2,n3);
			break;
	}
	return true;
}
