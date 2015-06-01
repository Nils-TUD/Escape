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

#ifdef __cplusplus
extern "C" {
#endif

void yyerror(char const *s);
int yyparse(void);
int yylex_destroy(void);

void pattern_destroy(void *e);

void *pattern_createGroup(void *list);
void *pattern_createList(bool group);
void pattern_addToList(void *list,void *elem);

void *pattern_createChar(char c);
void *pattern_createDot(void);
void *pattern_createRepeat(void *elem,int min,int max);

void *pattern_createChoice(void *list);

void *pattern_createCharClass(void *list,bool negate);
void pattern_addToCharClassList(void *list,void *elem);
void *pattern_createCharClassList(void);
void *pattern_createCharClassElem(char begin,char end);

#ifdef __cplusplus
}
#endif
