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

#pragma once

#include <esc/common.h>
#include <esc/fsinterface.h>

/* represents a not-printable character */
#define NPRINT			'\0'

/* an entry in the keymap */
typedef struct {
	char def;
	char shift;
	char alt;
} sKeymapEntry;

/* a keymap */
typedef struct {
	char path[MAX_PATH_LEN];
	ulong refs;
	sKeymapEntry *entries;
} sKeymap;

/**
 * Requests the keymap-object for the given file. If it already exists, the existing instance is
 * returned. Otherwise, it parses the given file and creates a new object.
 *
 * @param file the keymap-file
 * @return the keymap (or NULL)
 */
sKeymap *km_request(const char *file);

/**
 * Releases the given keymap
 *
 * @param map the keymap
 */
void km_release(sKeymap *map);

/**
 * Translates the given keycode + isBreak to the corresponding modifiers and character
 * with the given keymap.
 *
 * @param map the keymap to use
 * @param isBreak whether it is a breakcode
 * @param keycode the keycode
 * @param modifier will be set to the current modifiers
 * @return the character
 */
char km_translateKeycode(sKeymap *map,bool isBreak,uchar keycode,uchar *modifier);
