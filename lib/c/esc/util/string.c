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

#include <esc/common.h>
#include <esc/mem/heap.h>
#include <esc/util/string.h>
#include <string.h>
#include <assert.h>

#define INIT_SIZE		16

static void str_grow(sString *s,u32 reqSize);

sString *str_create(void) {
	sString *s = (sString*)heap_alloc(sizeof(sString));
	s->len = 0;
	s->size = INIT_SIZE;
	s->str = (char*)heap_alloc(s->size);
	*s->str = '\0';
	return s;
}

sString *str_link(char *str) {
	sString *s = (sString*)heap_alloc(sizeof(sString));
	s->len = strlen(str);
	s->size = 0;
	s->str = str;
	return s;
}

sString *str_copy(const char *str) {
	sString *s = (sString*)heap_alloc(sizeof(sString));
	s->len = strlen(str);
	s->size = s->len + 1;
	s->str = (char*)heap_alloc(s->size);
	strcpy(s->str,str);
	return s;
}

void str_appendc(sString *s,char c) {
	char str[2];
	str[0] = c;
	str[1] = '\0';
	str_append(s,str);
}

void str_append(sString *s,const char *str) {
	u32 len = strlen(str);
	str_grow(s,s->len + len + 1);
	strcpy(s->str + s->len,str);
	s->len += len;
}

void str_insertc(sString *s,u32 index,char c) {
	char str[2];
	str[0] = c;
	str[1] = '\0';
	str_insert(s,index,str);
}

void str_insert(sString *s,u32 index,const char *str) {
	u32 len = strlen(str);
	char *begin;
	assert(index <= s->len);
	str_grow(s,s->len + len + 1);
	begin = s->str + index;
	if(index <= s->len)
		memmove(begin + len,begin,s->len - index + 1);
	memcpy(begin,str,len);
	s->len += len;
}

s32 str_find(sString *s,const char *str,u32 offset) {
	char *match;
	assert(offset < s->len);
	match = strstr(s->str + offset,str);
	if(match)
		return match - s->str;
	return -1;
}

void str_delete(sString *s,u32 start,u32 count) {
	assert(start < s->len);
	assert(start + count <= s->len && start + count >= start);
	if(start + count <= s->len)
		memmove(s->str + start,s->str + start + count,s->len - (start + count) + 1);
	s->len -= count;
}

void str_destroy(sString *s) {
	if(s->size)
		heap_free(s->str);
	heap_free(s);
}

void str_resize(sString *s,u32 size) {
	assert(size > 0 && s->size > 0);
	if(s->size == size)
		return;
	if(size <= s->len) {
		s->len = size - 1;
		s->str[s->len] = '\0';
	}
	s->size = size;
	s->str = heap_realloc(s->str,size);
}

static void str_grow(sString *s,u32 reqSize) {
	assert(s->size > 0);
	if(s->size < reqSize) {
		s->size = MAX(s->size * 2,reqSize);
		s->str = heap_realloc(s->str,s->size);
	}
}
