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

#include <sys/common.h>
#include <sys/mem/kheap.h>
#include <sys/dbg/lines.h>
#include <sys/video.h>
#include <errors.h>

s32 lines_create(sLines *l) {
	l->lineCount = 0;
	l->linePos = 0;
	l->lineSize = 16;
	l->lines = (char**)kheap_alloc(l->lineSize * sizeof(char*));
	if(!l->lines)
		return ERR_NOT_ENOUGH_MEM;
	l->lines[l->lineCount] = kheap_alloc(VID_COLS + 1);
	if(!l->lines[l->lineCount]) {
		lines_destroy(l);
		return ERR_NOT_ENOUGH_MEM;
	}
	return 0;
}

void lines_append(sLines *l,char c) {
	if(l->linePos < VID_COLS)
		l->lines[l->lineCount][l->linePos++] = c;
}

s32 lines_newline(sLines *l) {
	u32 i;
	/* fill up with spaces */
	for(i = l->linePos; i < VID_COLS; i++)
		l->lines[l->lineCount][i] = ' ';
	l->lines[l->lineCount][i] = '\0';
	/* to next line */
	l->linePos = 0;
	l->lineCount++;
	/* allocate more lines if necessary */
	if(l->lineCount >= l->lineSize) {
		l->lineSize *= 2;
		l->lines = (char**)kheap_realloc(l->lines,l->lineSize * sizeof(char*));
		if(!l->lines)
			return ERR_NOT_ENOUGH_MEM;
	}
	/* allocate new line */
	l->lines[l->lineCount] = kheap_alloc(VID_COLS + 1);
	if(!l->lines[l->lineCount])
		return ERR_NOT_ENOUGH_MEM;
	return 0;
}

void lines_end(sLines *l) {
	u32 i;
	for(i = l->linePos; i < VID_COLS; i++)
		l->lines[l->lineCount][i] = ' ';
	l->lines[l->lineCount][i] = '\0';
	l->lineCount++;
}

void lines_destroy(sLines *l) {
	if(l->lines) {
		u32 i;
		for(i = 0; i < l->lineCount; i++)
			kheap_free(l->lines[i]);
		kheap_free(l->lines);
	}
}
