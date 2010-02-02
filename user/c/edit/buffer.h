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

#ifndef BUFFER_H_
#define BUFFER_H_

#include <esc/common.h>
#include <sllist.h>

typedef struct {
	char *str;
	u32 size;
	u32 length;
} sLine;

void buf_open(const char *file);

u32 buf_getLineCount(void);

sSLList *buf_getLines(void);

void buf_insertAt(s32 col,s32 row,char c);

void buf_newLine(s32 row);

void buf_removePrev(s32 col,s32 row);

void buf_removeCur(s32 col,s32 row);

void buf_store(const char *file);

#endif /* BUFFER_H_ */
