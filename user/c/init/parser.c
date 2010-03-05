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
#include <esc/heap.h>
#include <string.h>
#include <sllist.h>
#include "parser.h"
#include "driver.h"

/**
 * Parses one driver from the given line (until newline or \0) and fills the
 * given driver-load
 *
 * @param line the line-beginning
 * @param drv the driver-load to fill
 * @return the position where to continue
 */
static char *parseDriver(char *line,sDriverLoad *drv);

sDriverLoad **parseDrivers(char *drvFile) {
	u32 drvSize = 8;
	u32 drvPos = 0;
	sDriverLoad **drvBak;
	sDriverLoad **drvs;
	sDriverLoad **drv;
	sDriverLoad *s;
	char *str;

	/* create array */
	drvs = (sDriverLoad**)malloc(drvSize * sizeof(sDriverLoad*));
	if(drvs == NULL)
		return NULL;

	drvBak = drvs;
	str = drvFile;
	while(*str) {
		/* ";" at the line-start is a comment */
		if(*str != '#') {
			/* increase array-size? leave one slot for NULL */
			if(drvPos >= drvSize - 1) {
				drvSize += 8;
				drvs = (sDriverLoad**)realloc(drvs,drvSize * sizeof(sDriverLoad*));
				if(drvs == NULL)
					goto failed;
			}

			/* alloc mem for driver-load */
			s = (sDriverLoad*)malloc(sizeof(sDriverLoad));
			if(s == NULL)
				goto failed;

			/* parse this line */
			str = parseDriver(str,s);
			if(str == NULL)
				goto failed;
			/* if the line was empty (name == NULL), simply ignore it */
			if(s->name != NULL)
				drvs[drvPos++] = s;
			else
				free(s);
		}

		/* wait until newline */
		while(*str && *str != '\n')
			str++;
		if(!*str)
			break;
		str++;
	}

	/* terminate */
	drvs[drvPos] = NULL;
	return drvs;

failed:
	/* we have to free the already allocated mem */
	drv = drvBak + drvPos - 1;
	while(drv > drvBak) {
		free((*drv)->name);
		free((*drv)->deps);
		free((*drv)->waits);
		free(*drv);
		drv--;
	}
	free(drvBak);
	return NULL;
}

static char *parseDriver(char *line,sDriverLoad *drv) {
	char *s,*brk,*lineStart;
	u32 i,len,size;
	u8 *count;
	char ***array;

	/* search for name-end */
	s = strpbrk(line,"; \t\n");
	if(s == NULL)
		s = line + strlen(line);

	/* empty name? */
	if(s - line == 0)
		return s;

	/* set name */
	drv->name = (char*)malloc(s - line + 1);
	if(drv->name == NULL)
		return false;
	strncpy(drv->name,line,s - line);
	drv->name[s - line] = '\0';

	/* create wait-array */
	drv->waitCount = 0;
	drv->waits = (char**)malloc(8 * sizeof(char*));
	if(drv->waits == NULL) {
		free(drv->name);
		return NULL;
	}

	/* create dep-array */
	drv->depCount = 0;
	drv->deps = (char**)malloc(8 * sizeof(char*));
	if(drv->deps == NULL) {
		free(drv->name);
		free(drv->waits);
		return NULL;
	}

	/* no deps and waits? */
	if(*s == '\0' || *s == '\n')
		return s;

	/* first load waits, then load deps (syntax is the same) */
	line = s + 1;
	lineStart = line;
	for(i = 0; i < 2; i++) {
		size = 8;
		if(i == 0) {
			array = &drv->waits;
			count = &drv->waitCount;
		}
		else {
			array = &drv->deps;
			count = &drv->depCount;
		}

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
				/* increase array-size? */
				if(*count >= size) {
					size += 8;
					*array = (char**)realloc(*array,size * sizeof(char*));
					if(*array == NULL)
						goto failed;
				}

				/* copy wait-name */
				(*array)[*count] = (char*)malloc(len + 1);
				if((*array)[*count] == NULL)
					goto failed;
				strncpy((*array)[*count],line,len);
				(*array)[*count][len] = '\0';
				(*count)++;
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
	}

	return line;

failed:
	free(drv->name);
	free(drv->deps);
	free(drv->waits);
	return NULL;
}
