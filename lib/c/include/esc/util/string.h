/**
 * $Id: string.h 641 2010-05-02 18:24:30Z nasmussen $
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

#ifndef ESCSTRING_H_
#define ESCSTRING_H_

#include <esc/common.h>

/* All elements are READ-ONLY for the public! */
typedef struct {
/* public: */
	/**
	 * Pointer to heap-allocated string
	 */
	char *str;
	/**
	 * The number of chars (without '\0') in the string
	 */
	u32 len;
	/**
	 * The size of the heap-area
	 */
	u32 size;
} sString;

/**
 * Creates a new string-instance
 *
 * @return the string
 */
sString *str_create(void);

/**
 * Creates a new string-instance that links to the given string and is therefore immutable!
 *
 * @param str the string
 * @return the string
 */
sString *str_link(char *str);

/**
 * Creates a new string-instance and copies the given string into it
 *
 * @param str the string
 * @return the string
 */
sString *str_copy(const char *str);

/**
 * Appends the given character to the string
 *
 * @param s the string
 * @param c the character
 */
void str_appendc(sString *s,char c);

/**
 * Appends the given string
 *
 * @param s the string-instance
 * @param str the string to append
 */
void str_append(sString *s,const char *str);

/**
 * Inserts the given character at <index> into the string
 *
 * @param s the string
 * @param index the index
 * @param c the character
 */
void str_insertc(sString *s,u32 index,char c);

/**
 * Inserts <str> at <index>
 *
 * @param s the string-instance
 * @param index the index
 * @param str the string
 */
void str_insert(sString *s,u32 index,const char *str);

/**
 * Searches for the given string, beginning at <offset>
 *
 * @param s the string-instance
 * @param str the string to find
 * @param offset the offset where to begin
 * @return the position if found or -1
 */
s32 str_find(sString *s,const char *str,u32 offset);

/**
 * Deletes <start> to <start> + <count> - 1 from the string
 *
 * @param s the string-instance
 * @param start the start-position
 * @param count the number of chars to remove
 */
void str_delete(sString *s,u32 start,u32 count);

/**
 * Resizes the internal heap-area to <size>. Cuts the string appropriatly, if necessary.
 *
 * @param s the string-instance
 * @param size the size
 */
void str_resize(sString *s,u32 size);

/**
 * Destroys the given string
 *
 * @param s the string-instance
 */
void str_destroy(sString *s);

#endif /* ESCSTRING_H_ */
