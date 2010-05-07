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
#include <esc/keycodes.h>
#include <esc/io.h>
#include <esccodes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "keymap.h"

#define KEYMAP_SIZE		128

static bool km_parseLine(FILE *f,sKeymapEntry *map);
static char km_parseKey(FILE *f);
static char km_getKey(char *str);
static bool km_skipTrash(FILE *f);

static bool shiftDown = false;
static bool altDown = false;
static bool ctrlDown = false;

sKeymapEntry *km_parse(const char *file) {
	FILE *f;
	sKeymapEntry *map = (sKeymapEntry*)calloc(KEYMAP_SIZE,sizeof(sKeymapEntry));
	if(!map)
		return NULL;
	f = fopen(file,"r");
	if(!f) {
		free(map);
		return NULL;
	}
	while(1) {
		if(!km_parseLine(f,map))
			break;
	}
	fclose(f);
	return map;
}

char km_translateKeycode(sKeymapEntry *map,bool isBreak,u32 keycode,u8 *modifier) {
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

	e = map + keycode;
	*modifier = (altDown ? STATE_ALT : 0) | (ctrlDown ? STATE_CTRL : 0) |
			(shiftDown ? STATE_SHIFT : 0);
	if(shiftDown)
		return e->shift;
	if(altDown)
		return e->alt;
	return e->def;
}

static bool km_parseLine(FILE *f,sKeymapEntry *map) {
	u32 i,no;
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
		u32 code;
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
