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
#include <dirent.h>
#include <sys/sllist.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "mem.h"
#include "buffer.h"
#include "display.h"

#define INITIAL_LINE_SIZE	16

static sLine *buf_createLine(void);
static sLine *buf_readLine(FILE *f,bool *reachedEOF);

static sFileBuffer buf;

void buf_open(const char *file) {
	buf.lines = esll_create();
	buf.modified = false;
	if(file) {
		buf.filename = (char*)emalloc(strlen(file) + 1);
		strcpy(buf.filename,file);
	}
	else
		buf.filename = NULL;

	if(!file) {
		/* create at least one line */
		sLine *line = buf_createLine();
		esll_append(buf.lines,line);
	}
	else {
		FILE *f;
		sLine *line;
		bool reachedEOF = false;
		f = fopen(file,"r");
		if(!f)
			error("Unable to open '%s'",file);
		while(!reachedEOF) {
			line = buf_readLine(f,&reachedEOF);
			esll_append(buf.lines,line);
		}
		fclose(f);
	}
}

size_t buf_getLineCount(void) {
	return sll_length(buf.lines);
}

sFileBuffer *buf_get(void) {
	return &buf;
}

void buf_insertAt(int col,int row,char c) {
	sLine *line;
	assert(row >= 0 && row < (int)sll_length(buf.lines));
	line = (sLine*)sll_get(buf.lines,row);
	assert(col >= 0 && col <= (int)line->length);
	if(line->length >= line->size - 1) {
		line->size *= 2;
		line->str = (char*)erealloc(line->str,line->size);
	}
	if(col < (int)line->length)
		memmove(line->str + col + 1,line->str + col,line->length - col);
	line->str[col] = c;
	line->str[++line->length] = '\0';
	line->displLen += displ_getCharLen(c);
	buf.modified = true;
}

void buf_newLine(int col,int row) {
	sLine *cur,*line;
	assert(row < (int)sll_length(buf.lines));
	cur = (sLine*)sll_get(buf.lines,row);
	line = buf_createLine();
	if(col < (int)cur->length) {
		size_t i,moveLen = cur->length - col;
		/* append to next and remove from current */
		if(line->size - line->length <= moveLen) {
			line->size += moveLen;
			line->str = (char*)erealloc(line->str,line->size);
		}
		strncpy(line->str + line->length,cur->str + col,moveLen);
		for(i = col; i < cur->length; i++) {
			size_t clen = displ_getCharLen(cur->str[i]);
			line->displLen += clen;
			cur->displLen -= clen;
		}
		line->length += moveLen;
		cur->length -= moveLen;
		line->str[line->length] = '\0';
		cur->str[cur->length] = '\0';
	}
	esll_insert(buf.lines,line,row + 1);
	buf.modified = true;
}

void buf_moveToPrevLine(int row) {
	sLine *cur,*prev;
	assert(row > 0 && row < (int)sll_length(buf.lines));
	cur = (sLine*)sll_get(buf.lines,row);
	prev = (sLine*)sll_get(buf.lines,row - 1);
	/* append to prev */
	if(prev->size - prev->length <= cur->length) {
		prev->size += cur->length;
		prev->str = (char*)erealloc(prev->str,prev->size);
	}
	strcpy(prev->str + prev->length,cur->str);
	prev->displLen += cur->displLen;
	prev->length += cur->length;
	/* remove current row */
	sll_removeIndex(buf.lines,row);
	efree(cur->str);
	efree(cur);
	buf.modified = true;
}

void buf_removeCur(int col,int row) {
	sLine *line;
	assert(row >= 0 && row < (int)sll_length(buf.lines));
	line = (sLine*)sll_get(buf.lines,row);
	assert(col >= 0 && col <= (int)line->length);
	col++;
	if(col > (int)line->length)
		return;
	line->displLen -= displ_getCharLen(line->str[col - 1]);
	if(col < (int)line->length)
		memmove(line->str + col - 1,line->str + col,line->length - col);
	line->str[--line->length] = '\0';
	buf.modified = true;
}

static sLine *buf_createLine(void) {
	sLine *line = (sLine*)emalloc(sizeof(sLine));
	line->length = 0;
	line->displLen = 0;
	line->size = INITIAL_LINE_SIZE;
	line->str = (char*)emalloc(line->size);
	return line;
}

static sLine *buf_readLine(FILE *f,bool *reachedEOF) {
	char c;
	sLine *line = (sLine*)emalloc(sizeof(sLine));
	line->size = INITIAL_LINE_SIZE;
	line->length = 0;
	line->displLen = 0;
	line->str = (char*)emalloc(INITIAL_LINE_SIZE);
	while((c = getc(f)) != EOF && c != '\n') {
		line->displLen += displ_getCharLen(c);
		line->str[line->length++] = c;
		/* +1 for null-termination */
		if(line->length >= line->size - 1) {
			line->size *= 2;
			line->str = (char*)erealloc(line->str,line->size);
		}
	}
	*reachedEOF = c == EOF;
	line->str[line->length] = '\0';
	return line;
}

void buf_store(const char *file) {
	FILE *f;
	sLine *line;
	sSLNode *n;
	f = fopen(file,"w");
	if(!f)
		printe("Unable to store to '%s'",file);
	else {
		for(n = sll_begin(buf.lines); n != NULL; n = n->next) {
			line = (sLine*)n->data;
			fwrite(line->str,sizeof(char),line->length,f);
			if(n->next != NULL)
				fwrite("\n",sizeof(char),1,f);
		}
		fclose(f);
	}
}
