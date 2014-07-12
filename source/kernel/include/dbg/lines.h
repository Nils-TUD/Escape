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

#include <common.h>
#include <cppsupport.h>

class Lines : public CacheAllocatable {
public:
	/**
	 * Creates a new lines-object
	 */
	explicit Lines() : lines(0), lineCount(0), linePos(0), lineSize(0) {
	}

	/**
	 * Free's the memory
	 */
	~Lines();

	/**
	 * @return true if no error happened so far
	 */
	bool isValid() const {
		return lineSize != (size_t)-1;
	}
	/**
	 * @return the number of finished lines
	 */
	size_t getLineCount() const {
		return lineCount;
	}
	/**
	 * @param i the line number
	 * @return the line i
	 */
	const char *getLine(size_t i) const {
		return lines[i];
	}

	/**
	 * Appends the given string to the current line, if possible
	 *
	 * @param str the string
	 */
	void appendStr(const char *str);

	/**
	 * Appends the character to the current line, if possible
	 *
	 * @param c the character
	 */
	void append(char c);

	/**
	 * Adds a new line
	 *
	 * @return true if successfull
	 */
	bool newLine();

	/**
	 * Ends the current line. This is intended for finalizing, i.e. you should call it when everything
	 * has been added. It will finish the current line and increase the l->lineCount by 1, so that its
	 * really the number of lines and not the index of the current line.
	 */
	void endLine();

private:
	bool init();

	char **lines;
	size_t lineCount;
	size_t linePos;
	size_t lineSize;
};
