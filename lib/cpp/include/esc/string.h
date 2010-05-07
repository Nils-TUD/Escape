/**
 * $Id: string.h 244 2009-06-20 16:35:38Z nasmussen $
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

#ifndef CPP_STRING_H_
#define CPP_STRING_H_

#include <esc/common.h>
#include <esc/stream.h>
#include <string.h>

namespace esc {
	/**
	 * Supports the basic string-operations like operator[], append, insert, erase, length
	 * and so on.
	 */
	class String {
		typedef char *pChar;
		friend Stream &operator<<(Stream &s,String &str);

	private:
		/**
		 * Default initial size
		 */
		static const u32 initSize = 8;

	public:
		/**
		 * Represents an invalid position in the string. Will be used for e.g. find()
		 */
		static const u32 npos = -1;

	public:
		/**
		 * Creates a new string with an initial space for <size> characters
		 *
		 * @param size the initial size (default 8)
		 */
		String(u32 size = initSize);

		/**
		 * Copy-constructor
		 */
		String(const String &s);

		/**
		 * Creates a string from the given c-string
		 *
		 * @param s the c-string
		 */
		String(const char *s);

		/**
		 * Destructor
		 */
		~String();

		/**
		 * @return the length of the string (without null-termination)
		 */
		inline u32 length() const {
			return _length;
		};

		/**
		 * Ensures that the string has space for at least <size> characters
		 *
		 * @param size the min-space you need
		 */
		void reserve(u32 size);

		/**
		 * Searches for the given characters, starting at <offset>
		 *
		 * @param c the character
		 * @param offset the offset to start at (default 0)
		 * @return the position of the character or String::npos if not found
		 */
		u32 find(char c,u32 offset = 0) const;
		/**
		 * Searches for the given string, starting at <offset>
		 *
		 * @param sub the string to find
		 * @param offset the offset to start at (default 0)
		 * @return the position of the string or String::npos if not found
		 */
		u32 find(const char *sub,u32 offset = 0) const;

		/**
		 * Inserts <c> at <offset>
		 *
		 * @param offset the offset in the string (0..<length>)
		 * @param c the character
		 */
		void insert(u32 offset,char c);

		/**
		 * Inserts <s> at <offset>
		 *
		 * @param offset the offset in the string (0..<length>)
		 * @param s the string
		 */
		void insert(u32 offset,const String &s);

		/**
		 * Appends the given character
		 *
		 * @param c the character
		 */
		inline void append(char c) {
			operator +=(c);
		};

		/**
		 * Appends the given string
		 *
		 * @param s the string
		 */
		inline void append(const String &s) {
			operator +=(s);
		};

		/**
		 * Deletes all characters, starting at <offset>
		 *
		 * @param offset the offset at which to start deletion
		 */
		inline void erase(u32 offset) {
			erase(offset,_length - offset);
		};

		/**
		 * Deletes <count> characters, starting at <offset>
		 *
		 * @param offset the offset at which to start deletion
		 * @param count the number of chars to delete
		 */
		void erase(u32 offset,u32 count);

		/**
		 * @return the internal c-string
		 */
		inline const char *c_str() const {
			return _str;
		};

		/**
		 * Assignment operator
		 */
		String &operator=(const String &s);
		/**
		 * Appends <c>
		 */
		String &operator+=(char c);
		/**
		 * Appends <s>
		 */
		String &operator+=(const String &s);

		/**
		 * Compares the string to <s>
		 */
		inline bool operator==(const String &s) const {
			return _length == s._length && strcmp(_str,s._str) == 0;
		};
		/**
		 * Compares the string to <s>
		 */
		inline bool operator!=(const String &s) const {
			return _length != s._length || strcmp(_str,s._str) != 0;
		};

		/**
		 * Returns the character at given index.
		 * NOTE: At the moment with no bounds-check!
		 *
		 * @param index the index
		 * @return the char at <index>
		 */
		inline char operator[](u32 index) const {
			return _str[index];
		};

	private:
		/**
		 * Increases the size to have space for <min> chars (without null-termination)
		 *
		 * @param min the required space
		 */
		void increaseSize(u32 min);

		/**
		 * Character-buffer
		 */
		char *_str;
		/**
		 * length of the string (without null-termination)
		 */
		u32 _length;
		/**
		 * size of the buffer
		 */
		u32 _size;
	};

	/**
	 * Prints <str> to <s>
	 *
	 * @param s the stream
	 * @param str the string
	 * @return the stream
	 */
	Stream &operator<<(Stream &s,String &str);
}

#endif /* CPP_STRING_H_ */
