/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/mem/cache.h>
#include <sys/dbg/lines.h>
#include <sys/video.h>
#include <errno.h>

bool Lines::init() {
	if(lineSize == (size_t)-1)
		return false;
	if(lineSize == 0)
		return newLine();
	return true;
}

void Lines::appendStr(const char *str) {
	if(!init())
		return;
	while(*str)
		append(*str++);
}

void Lines::append(char c) {
	if(!init())
		return;
	if(linePos < VID_COLS)
		lines[lineCount][linePos++] = c;
}

bool Lines::newLine() {
	if(lineSize == (size_t)-1)
		return false;
	if(lineSize) {
		/* fill up with spaces */
		size_t i;
		for(i = linePos; i < VID_COLS; i++)
			lines[lineCount][i] = ' ';
		lines[lineCount][i] = '\0';
		/* to next line */
		linePos = 0;
		lineCount++;
	}
	/* allocate more lines if necessary */
	if(lineCount >= lineSize) {
		char **newlines;
		lineSize = lineSize == 0 ? 16 : lineSize * 2;
		newlines = (char**)Cache::realloc(lines,lineSize * sizeof(char*));
		if(!newlines) {
			lineSize = (size_t)-1;
			return false;
		}
		lines = newlines;
	}
	/* allocate new line */
	lines[lineCount] = (char*)Cache::alloc(VID_COLS + 1);
	if(!lines[lineCount]) {
		if(lineCount)
			lineCount--;
		lineSize = (size_t)-1;
		return false;
	}
	return true;
}

void Lines::endLine() {
	if(!init())
		return;
	size_t i;
	for(i = linePos; i < VID_COLS; i++)
		lines[lineCount][i] = ' ';
	lines[lineCount][i] = '\0';
	lineCount++;
}

Lines::~Lines() {
	if(lines) {
		for(size_t i = 0; i < lineCount; i++)
			Cache::free(lines[i]);
		Cache::free(lines);
	}
}
