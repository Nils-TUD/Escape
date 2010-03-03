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
#include "service.h"

/**
 * Parses one service from the given line (until newline or \0) and fills the
 * given service-load
 *
 * @param line the line-beginning
 * @param serv the service-load to fill
 * @return the position where to continue
 */
static char *parseService(char *line,sServiceLoad *serv);

sServiceLoad **parseServices(char *servFile) {
	u32 servSize = 8;
	u32 servPos = 0;
	sServiceLoad **servBak;
	sServiceLoad **servs;
	sServiceLoad **serv;
	sServiceLoad *s;
	char *str;

	/* create array */
	servs = (sServiceLoad**)malloc(servSize * sizeof(sServiceLoad*));
	if(servs == NULL)
		return NULL;

	servBak = servs;
	str = servFile;
	while(*str) {
		/* ";" at the line-start is a comment */
		if(*str != '#') {
			/* increase array-size? leave one slot for NULL */
			if(servPos >= servSize - 1) {
				servSize += 8;
				servs = (sServiceLoad**)realloc(servs,servSize * sizeof(sServiceLoad*));
				if(servs == NULL)
					goto failed;
			}

			/* alloc mem for service-load */
			s = (sServiceLoad*)malloc(sizeof(sServiceLoad));
			if(s == NULL)
				goto failed;

			/* parse this line */
			str = parseService(str,s);
			if(str == NULL)
				goto failed;
			/* if the line was empty (name == NULL), simply ignore it */
			if(s->name != NULL)
				servs[servPos++] = s;
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
	servs[servPos] = NULL;
	return servs;

failed:
	/* we have to free the already allocated mem */
	serv = servBak + servPos - 1;
	while(serv > servBak) {
		free((*serv)->name);
		free((*serv)->deps);
		free((*serv)->waits);
		free(*serv);
		serv--;
	}
	free(servBak);
	return NULL;
}

static char *parseService(char *line,sServiceLoad *serv) {
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
	serv->name = (char*)malloc(s - line + 1);
	if(serv->name == NULL)
		return false;
	strncpy(serv->name,line,s - line);
	serv->name[s - line] = '\0';

	/* create wait-array */
	serv->waitCount = 0;
	serv->waits = (char**)malloc(8 * sizeof(char*));
	if(serv->waits == NULL) {
		free(serv->name);
		return NULL;
	}

	/* create dep-array */
	serv->depCount = 0;
	serv->deps = (char**)malloc(8 * sizeof(char*));
	if(serv->deps == NULL) {
		free(serv->name);
		free(serv->waits);
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
			array = &serv->waits;
			count = &serv->waitCount;
		}
		else {
			array = &serv->deps;
			count = &serv->depCount;
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
	free(serv->name);
	free(serv->deps);
	free(serv->waits);
	return NULL;
}
