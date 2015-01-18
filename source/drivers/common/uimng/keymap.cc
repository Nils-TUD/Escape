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

#include <esc/proto/input.h>
#include <sys/common.h>
#include <sys/esccodes.h>
#include <sys/io.h>
#include <sys/keycodes.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "keymap.h"

const char *Keymap::KEYMAP_FILE = "/etc/keymap";

bool Keymap::_shiftDown = false;
bool Keymap::_altDown = false;
bool Keymap::_ctrlDown = false;
std::vector<Keymap*> Keymap::_maps;
Keymap *Keymap::_defMap = NULL;

Keymap *Keymap::getDefault() {
	static char path[MAX_PATH_LEN];
	if(_defMap == NULL) {
		/* determine default keymap */
		char *newline;
		FILE *f = fopen(KEYMAP_FILE,"r");
		if(f == NULL)
			error("Unable to open %s",KEYMAP_FILE);
		fgets(path,MAX_PATH_LEN,f);
		if((newline = strchr(path,'\n')))
			*newline = '\0';
		fclose(f);

		/* load default map */
		_defMap = request(path);
		if(!_defMap)
			error("Unable to load default keymap");
	}
	return _defMap;
}

Keymap *Keymap::request(const std::string &kmfile) {
	/* do we already have the keymap? */
	for(auto m = _maps.begin(); m != _maps.end(); ++m) {
		if((*m)->_file == kmfile) {
			(*m)->_refs++;
			return *m;
		}
	}

	/* create map */
	FILE *f = fopen(kmfile.c_str(),"r");
	if(!f)
		return NULL;
	Keymap *map = new Keymap(kmfile);
	while(1) {
		if(!parseLine(f,map->_entries))
			break;
	}
	fclose(f);
	_maps.push_back(map);
	return map;
}

void Keymap::release(Keymap *map) {
	if(--map->_refs == 0) {
		_maps.erase_first(map);
		delete map;
	}
}

char Keymap::translateKeycode(uchar flags,uchar keycode,uchar *modifier) const {
	Entry *e;
	/* handle shift, alt and ctrl */
	bool isBreak = !!(flags & esc::Keyb::Event::FL_BREAK);
	switch(keycode) {
		case VK_LSHIFT:
		case VK_RSHIFT:
			_shiftDown = !isBreak;
			break;
		case VK_LALT:
		case VK_RALT:
			_altDown = !isBreak;
			break;
		case VK_LCTRL:
		case VK_RCTRL:
			_ctrlDown = !isBreak;
			break;
	}

	e = _entries + keycode;
	*modifier = (_altDown ? STATE_ALT : 0) | (_ctrlDown ? STATE_CTRL : 0) |
			(_shiftDown ? STATE_SHIFT : 0) | (isBreak ? STATE_BREAK : 0) |
			((flags & esc::Keyb::Event::FL_CAPS) ? STATE_CAPS : 0);
	if(_shiftDown || (flags & esc::Keyb::Event::FL_CAPS))
		return e->shift;
	if(_altDown)
		return e->alt;
	return e->def;
}

bool Keymap::parseLine(FILE *f,Entry *map) {
	size_t i;
	int no;
	char *entries[3];
	/* scan number */
	if(fscanf(f,"%d",&no) != 1 || (size_t)no >= KEYMAP_SIZE)
		return false;
	if(!skipTrash(f))
		return false;
	/* scan chars */
	entries[0] = &(map[no].def);
	entries[1] = &(map[no].shift);
	entries[2] = &(map[no].alt);
	for(i = 0; i < 3; i++) {
		char c = parseKey(f);
		if(c == EOF)
			return false;
		*(entries[i]) = c;
		if(!skipTrash(f))
			return false;
	}
	return true;
}

char Keymap::parseKey(FILE *f) {
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
		return getKey(str);
	}
	else if(c == 'x') {
		uint code;
		fscanf(f,"%2x",&code);
		return (char)code;
	}
	return NPRINT;
}

char Keymap::getKey(char *str) {
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

bool Keymap::skipTrash(FILE *f) {
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
