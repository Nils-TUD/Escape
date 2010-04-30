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

#ifndef ESCSTRING_H_
#define ESCSTRING_H_

#include <esc/common.h>

/* All elements are READ-ONLY for the public! */
typedef struct {
	char *str;
	u32 len;
	u32 size;
} sString;

sString *str_create(void);
sString *str_link(char *str);
sString *str_copy(const char *str);
void str_append(sString *s,const char *str);
void str_insert(sString *s,u32 index,const char *str);
s32 str_find(sString *s,const char *str,u32 offset);
void str_delete(sString *s,u32 start,u32 count);
void str_resize(sString *s,u32 size);
void str_destroy(sString *s);

#endif /* ESCSTRING_H_ */
