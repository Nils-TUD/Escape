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

#include <esc/common.h>
#include <esc/keycodes.h>
#include <esc/io.h>
#include <esc/esccodes.h>
#include <esc/sllist.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "keymap.h"

#define KEYMAP_SIZE		128
#define KEYMAP_FILE		"/etc/keymap"

static bool km_parseLine(FILE *f,sKeymapEntry *map);
static char km_parseKey(FILE *f);
static char km_getKey(char *str);
static bool km_skipTrash(FILE *f);

static bool shiftDown = false;
static bool altDown = false;
static bool ctrlDown = false;
static sSLList *maps;
static sKeymap *defmap = NULL;

sKeymap *km_getDefault(void) {
	static char defKeymapPath[MAX_PATH_LEN];
	if(defmap == NULL) {
		/* determine default keymap */
		char *newline;
		FILE *f = fopen(KEYMAP_FILE,"r");
		if(f == NULL)
			error("Unable to open %s",KEYMAP_FILE);
		fgets(defKeymapPath,MAX_PATH_LEN,f);
		if((newline = strchr(defKeymapPath,'\n')))
			*newline = '\0';
		fclose(f);

		/* load default map */
		defmap = km_request(defKeymapPath);
		if(!defmap)
			error("Unable to load default keymap");
	}
	return defmap;
}

sKeymap *km_request(const char *file) {
	if(maps == NULL) {
		maps = sll_create();
		if(maps == NULL)
			error("Unable to create maps-list");
	}

	/* do we already have the keymap? */
	sSLNode *n;
	for(n = sll_begin(maps); n != NULL; n = n->next) {
		sKeymap *m = (sKeymap*)n->data;
		if(strcmp(m->path,file) == 0) {
			m->refs++;
			return m;
		}
	}

	/* create map */
	FILE *f;
	sKeymap *map = (sKeymap*)malloc(sizeof(sKeymap));
	if(!map)
		return NULL;
	map->refs = 1;
	strnzcpy(map->path,file,sizeof(map->path));
	map->entries = static_cast<sKeymapEntry*>(calloc(KEYMAP_SIZE,sizeof(sKeymapEntry)));
	if(!map->entries)
		goto error;

	f = fopen(file,"r");
	if(!f)
		goto errorEntries;
	while(1) {
		if(!km_parseLine(f,map->entries))
			break;
	}
	fclose(f);

	if(!sll_append(maps,map))
		goto errorEntries;
	return map;

errorEntries:
	free(map->entries);
error:
	free(map);
	return NULL;
}

void km_release(sKeymap *map) {
	if(--map->refs == 0) {
		sll_removeFirstWith(maps,map);
		free(map->entries);
		free(map);
	}
}

char km_translateKeycode(const sKeymap *map,bool isBreak,uchar keycode,uchar *modifier) {
	sKeymapEntry *e;
	/* handle shift, alt and ctrl */
	switch(keycode) {
		case VK_LSHIFT:
		case VK_RSHIFT:
			shiftDown = !isBreak;
			break;
		case VK_LALT:
		case VK_RALT:
			altDown = !isBreak;
			break;
		case VK_LCTRL:
		case VK_RCTRL:
			ctrlDown = !isBreak;
			break;
	}

	e = map->entries + keycode;
	*modifier = (altDown ? STATE_ALT : 0) | (ctrlDown ? STATE_CTRL : 0) |
			(shiftDown ? STATE_SHIFT : 0) | (isBreak ? STATE_BREAK : 0);
	if(shiftDown)
		return e->shift;
	if(altDown)
		return e->alt;
	return e->def;
}

static bool km_parseLine(FILE *f,sKeymapEntry *map) {
	size_t i;
	int no;
	char *entries[3];
	/* scan number */
	if(fscanf(f,"%d",&no) != 1 || no >= KEYMAP_SIZE)
		return false;
	if(!km_skipTrash(f))
		return false;
	/* scan chars */
	entries[0] = &(map[no].def);
	entries[1] = &(map[no].shift);
	entries[2] = &(map[no].alt);
	for(i = 0; i < 3; i++) {
		char c = km_parseKey(f);
		if(c == EOF)
			return false;
		*(entries[i]) = c;
		if(!km_skipTrash(f))
			return false;
	}
	return true;
}

static char km_parseKey(FILE *f) {
	char str[3];
	char c = fgetc(f);
	if(c == '\'') {
		str[0] = fgetc(f);
		str[1] = fgetc(f);
		if(str[0] != '\\' && str[1] == '\'')
			str[1] = '\0';
		else {
			if(fgetc(f) != '\'')
				return EOF;
			str[2] = '\0';
		}
		return km_getKey(str);
	}
	else if(c == 'x') {
		uint code;
		fscanf(f,"%2x",&code);
		return (char)code;
	}
	return NPRINT;
}

static char km_getKey(char *str) {
	if(*str == '\\') {
		switch(*(str + 1)) {
			case 'b':
				return '\b';
			case 'n':
				return '\n';
			case 't':
				return '\t';
			case '\'':
				return '\'';
			case '\\':
				return '\\';
			default:
				return NPRINT;
		}
	}
	return *str;
}

static bool km_skipTrash(FILE *f) {
	bool comment = false;
	while(1) {
		char c = fgetc(f);
		if(c == EOF)
			return false;
		if(c == '#')
			comment = true;
		else {
			if(comment && c == '\n')
				comment = false;
			else if(!comment && !isspace(c)) {
				ungetc(c,f);
				break;
			}
		}
	}
	return true;
}
