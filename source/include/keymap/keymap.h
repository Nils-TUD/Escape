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

#pragma once

#include <sys/common.h>
#include <sys/keycodes.h>
#include <stdio.h>
#include <vector>

class Keymap {
	struct Entry {
		char def;
		char shift;
		char alt;
	};
	struct ReverseEntry {
		uchar keycode;
		uchar modifier;
	};
	struct EscapeCode {
		const char *seq;
		uchar keycode;
		uchar modifier;
		uchar hasChar;
	};

	/* represents a not-printable character */
	static const char NPRINT			= '\0';
	static const size_t KEYMAP_SIZE		= 128;
	static const char *KEYMAP_FILE;

	explicit Keymap(const std::string &f)
		: _file(f), _refs(1), _entries(new Entry[KEYMAP_SIZE]()),
		  _reventries(new ReverseEntry[KEYMAP_SIZE]()) {
	}
	~Keymap() {
		delete[] _reventries;
		delete[] _entries;
	}

public:
	/**
	 * @return the default keymap
	 */
	static Keymap *getDefault();

	/**
	 * Requests the keymap-object for the given file. If it already exists, the existing instance is
	 * returned. Otherwise, it parses the given file and creates a new object.
	 *
	 * @param f the keymap-file
	 * @return the keymap (or NULL)
	 */
	static Keymap *request(const std::string &f);

	/**
	 * Releases the given keymap
	 *
	 * @param map the keymap
	 */
	static void release(Keymap *map);

	/**
	 * @return the keymap-file
	 */
	const std::string &file() const {
		return _file;
	}

	/**
	 * Translate the given character to the corresponding keycode and modifiers.
	 *
	 * @param c the character
	 * @param modifier the modifier to set
	 * @return the keycode
	 */
	uchar translateChar(char c,uchar *modifier) const;

	/**
	 * Translates the given keycode + isBreak to the corresponding modifiers and character
	 * with the given keymap.
	 *
	 * @param map the keymap to use
	 * @param flags the flags
	 * @param keycode the keycode
	 * @param modifier will be set to the current modifiers
	 * @return the character
	 */
	char translateKeycode(uchar flags,uchar keycode,uchar *modifier) const;

private:
	static bool parseLine(FILE *f,Entry *map,ReverseEntry *rmap);
	static char parseKey(FILE *f);
	static char getKey(char *str);
	static bool skipTrash(FILE *f);

	std::string _file;
	ulong _refs;
	Entry *_entries;
	ReverseEntry *_reventries;

	static bool _shiftDown;
	static bool _altDown;
	static bool _ctrlDown;
	static std::vector<Keymap*> _maps;
	static Keymap *_defMap;
	static EscapeCode _escCodes[];
};
