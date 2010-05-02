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
#include <util/vector.h>
#include <mem/heap.h>
#include <string.h>
#include "iparser.h"
#include "idriver.h"

/**
 * Parses one driver from the given line (until newline or \0) and fills the
 * given driver-load
 *
 * @param line the line-beginning
 * @param drv the driver-load to fill
 * @return the position where to continue
 */
static char *parseDriver(char *line,sDriverLoad *drv);
/**
 * Parses a driver-list at the given position in the line and puts the names in the vector
 */
static char *parseDriverNames(sVector *vec,char *lineStart,char *line);

sVector *parseDrivers(char *drvFile) {
	sVector *drvs = vec_create(sizeof(sDriverLoad*));
	char *str = drvFile;
	while(*str) {
		/* ";" at the line-start is a comment */
		if(*str != '#') {
			/* alloc mem for driver-load */
			sDriverLoad *s = (sDriverLoad*)heap_alloc(sizeof(sDriverLoad));
			/* parse this line */
			str = parseDriver(str,s);
			/* if the line was empty (name == NULL), simply ignore it */
			if(s->name != NULL)
				vec_add(drvs,&s);
			else
				heap_free(s);
		}

		/* wait until newline */
		while(*str && *str != '\n')
			str++;
		if(!*str)
			break;
		str++;
	}
	return drvs;
}

static char *parseDriver(char *line,sDriverLoad *drv) {
	char *s,*lineStart;

	/* search for name-end */
	s = strpbrk(line,"; \t\n");
	if(s == NULL)
		s = line + strlen(line);

	/* empty name? */
	if(s - line == 0)
		return s;

	/* set name */
	drv->name = (char*)heap_alloc(s - line + 1);
	strncpy(drv->name,line,s - line);
	drv->name[s - line] = '\0';

	/* create wait-array */
	drv->waits = vec_create(sizeof(char*));
	drv->deps = vec_create(sizeof(char*));

	/* no deps and waits? */
	if(*s == '\0' || *s == '\n')
		return s;

	/* first load waits, then load deps (syntax is the same) */
	line = s + 1;
	lineStart = line;
	line = parseDriverNames(drv->waits,lineStart,line);
	line = parseDriverNames(drv->deps,lineStart,line);
	return line;
}

static char *parseDriverNames(sVector *vec,char *lineStart,char *line) {
	u32 len;
	char *brk;
	while(1) {
		/* skip whitespace */
		while(*line && *line != '\n' && (*line == ' ' || *line == '\t'))
			line++;

		/* search for sep */
		if(!*line || *line == '\n')
			break;
		brk = strpbrk(line,",;\t \n");
		/* if we're at the end, use all until the end */
		if(brk == NULL)
			brk = lineStart + strlen(lineStart);

		len = brk - line;
		if(len > 0) {
			char *name = (char*)heap_alloc(len + 1);
			strncpy(name,line,len);
			name[len] = '\0';
			vec_add(vec,&name);
		}

		/* continue behind the separator */
		line = brk + 1;
		/* if we are finished with this line or type (wait,dep), break */
		if(!*brk || *brk == '\n' || *brk == ';') {
			/* line finished? so we one step too far */
			if(*brk && *brk == '\n')
				line--;
			break;
		}
	}
	return line;
}
