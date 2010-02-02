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
#include <esc/fileio.h>
#include <esc/dir.h>
#include <sllist.h>
#include <assert.h>
#include <string.h>
#include "mem.h"
#include "buffer.h"

#define INITIAL_LINE_SIZE	16

static sLine *buf_createLine(void);
static sLine *buf_readLine(tFile *f,bool *reachedEOF);

static sSLList *lines = NULL;

void buf_open(const char *file) {
	lines = esll_create();

	if(!file) {
		/* create at least one line */
		sLine *line = buf_createLine();
		esll_append(lines,line);
	}
	else {
		char absp[MAX_PATH_LEN];
		tFile *f;
		sLine *line;
		bool reachedEOF = false;
		abspath(absp,MAX_PATH_LEN,file);
		f = fopen(absp,"r");
		if(!f)
			error("Unable to open '%s'",absp);
		while(!reachedEOF) {
			line = buf_readLine(f,&reachedEOF);
			esll_append(lines,line);
		}
		fclose(f);
	}
}

u32 buf_getLineCount(void) {
	return sll_length(lines);
}

sSLList *buf_getLines(void) {
	return lines;
}

void buf_insertAt(s32 col,s32 row,char c) {
	assert(row >= 0 && row < (s32)sll_length(lines));
	sLine *line = (sLine*)sll_get(lines,row);
	assert(col >= 0 && col <= (s32)line->length);
	if(line->length >= line->size - 1) {
		line->size *= 2;
		line->str = erealloc(line->str,line->size);
	}
	if(col < (s32)line->length)
		memmove(line->str + col + 1,line->str + col,line->length - col);
	line->str[col] = c;
	line->str[++line->length] = '\0';
}

void buf_newLine(s32 row) {
	assert(row < (s32)sll_length(lines));
	sLine *line = buf_createLine();
	sll_insert(lines,line,row + 1);
}

void buf_removePrev(s32 col,s32 row) {
	if(col > 0)
		buf_removeCur(col - 1,row);
}

void buf_removeCur(s32 col,s32 row) {
	assert(row >= 0 && row < (s32)sll_length(lines));
	sLine *line = (sLine*)sll_get(lines,row);
	assert(col >= 0 && col <= (s32)line->length);
	col++;
	if(col > (s32)line->length)
		return;
	if(col < (s32)line->length)
		memmove(line->str + col - 1,line->str + col,line->length - col);
	line->str[--line->length] = '\0';
}

static sLine *buf_createLine(void) {
	sLine *line = (sLine*)emalloc(sizeof(sLine));
	line->length = 0;
	line->size = INITIAL_LINE_SIZE;
	line->str = (char*)emalloc(line->size);
	return line;
}

static sLine *buf_readLine(tFile *f,bool *reachedEOF) {
	char c;
	sLine *line = (sLine*)emalloc(sizeof(sLine));
	line->size = INITIAL_LINE_SIZE;
	line->length = 0;
	line->str = (char*)emalloc(INITIAL_LINE_SIZE);
	while((c = fscanc(f)) != IO_EOF && c != '\n') {
		line->str[line->length++] = c;
		/* +1 for null-termination */
		if(line->length >= line->size - 1) {
			line->size *= 2;
			line->str = erealloc(line->str,line->size);
		}
	}
	*reachedEOF = c == IO_EOF;
	line->str[line->length] = '\0';
	return line;
}

void buf_store(const char *file) {
	UNUSED(file);
}
