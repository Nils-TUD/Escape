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
#include "mem.h"
#include "buffer.h"

#define INITIAL_LINE_SIZE	16

static sLine *buf_readLine(tFile *f,bool *reachedEOF);

static sSLList *lines = NULL;

void buf_open(const char *file) {
	lines = esll_create();

	if(!file) {
		/* create at least one line */
		sLine *line = (sLine*)emalloc(sizeof(sLine));
		line->length = 0;
		line->size = INITIAL_LINE_SIZE;
		line->str = (char*)emalloc(line->size);
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

sSLList *buf_getLines(void) {
	return lines;
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
