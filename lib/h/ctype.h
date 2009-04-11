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

#ifndef CTYPE_H_
#define CTYPE_H_

#include <types.h>

/**
 * @param c the character
 * @return non-zero if its argument is a numeric digit or a letter of the alphabet.
 * 	Otherwise, zero is returned.
 */
bool isalnum(s32 c);

/**
 * @param c the character
 * @return non-zero if its argument is a letter of the alphabet. Otherwise, zero is returned.
 */
bool isalpha(s32 c);

/**
 * @param c the character
 * @return non-zero if its argument is a control-character ( < 0x20 )
 */
bool iscntrl(s32 c);

/**
 * @param c the character
 * @return non-zero if its argument is a digit between 0 and 9. Otherwise, zero is returned.
 */
bool isdigit(s32 c);

/**
 * @param c the character
 * @return non-zero if its argument is a lowercase letter. Otherwise, zero is returned.
 */
bool islower(s32 c);

/**
 * @param c the character
 * @return non-zero if its argument is a printable character
 */
bool isprint(s32 c);

/**
 * @param c the character
 * @return non-zero if its argument is a punctuation character
 */
bool ispunct(s32 c);

/**
 * @param c the character
 * @return non-zero if its argument is some sort of space (i.e. single space, tab,
 * 	vertical tab, form feed, carriage return, or newline). Otherwise, zero is returned.
 */
bool isspace(s32 c);

/**
 * @param c the character
 * @return non-zero if its argument is an uppercase letter. Otherwise, zero is returned.
 */
bool isupper(s32 c);

/**
 *
 * @param c the character
 * @return non-zero if its argument is a hexadecimal digit
 */
bool isxdigit(s32 c);

/**
 * @param ch the char
 * @return the lowercase version of the character ch.
 */
s32 tolower(s32 ch);

/**
 * @param ch the char
 * @return the uppercase version of the character ch.
 */
s32 toupper(s32 ch);

#endif /* CTYPE_H_ */
