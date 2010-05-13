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

#ifndef LANG_H_
#define LANG_H_

typedef s32 tIntType;

extern int openBrk;
extern int openGraves;

/**
 * Resets the position
 */
void lang_reset(void);

/**
 * Marks the shell as "interrupted"
 */
void lang_setInterrupted(void);

/**
 * @return whether the shell has been interrupted
 */
bool lang_isInterrupted(void);

/**
 * Begins the given token (stores location)
 *
 * @param t the token
 */
void lang_beginToken(char *t);

/**
 * Error-reporting-function called by flex
 *
 * @param s the error-message
 */
void yyerror(char const *s,...);

#endif /* LANG_H_ */
