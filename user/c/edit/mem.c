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
#include <esc/heap.h>
#include <string.h>
#include <sllist.h>
#include "mem.h"

void *emalloc(u32 size) {
	void *p = malloc(size);
	if(!p)
		error("Unable to allocate %d bytes",size);
	return p;
}

void *erealloc(void *p,u32 size) {
	void *pn = realloc(p,size);
	if(!pn)
		error("Unable to realloc mem @ %x to %d bytes",p,size);
	return pn;
}

char *estrdup(const char *s) {
	char *dup = strdup(s);
	if(!dup)
		error("Unable to duplicate string '%s'",s);
	return dup;
}

char *estrndup(const char *s,u32 n) {
	char *dup = strndup(s,n);
	if(!dup)
		error("Unable to duplicate string '%s'",s);
	return dup;
}

sSLList *esll_create(void) {
	sSLList *list = sll_create();
	if(!list)
		error("Unable to create linked list");
	return list;
}

void esll_insert(sSLList *list,const void *data,u32 index) {
	if(!sll_insert(list,data,index))
		error("Unable to insert an element to the linked list");
}

void esll_append(sSLList *list,const void *data) {
	if(!sll_append(list,data))
		error("Unable to append an element to the linked list");
}

void efree(void *p) {
	free(p);
}
