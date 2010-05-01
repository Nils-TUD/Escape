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
#include <mem/heap.h>
#include <util/cmdargs.h>
#include <exceptions/outofmemory.h>
#include <exceptions/cmdargs.h>
#include <assert.h>
#include <sllist.h>
#include <string.h>

static void cmdargs_parse(sCmdArgs *a,const char *fmt,...);
static s32 cmdargs_find(sCmdArgs *a,char *begin,char *end);
static void cmdargs_setVal(sCmdArgs *a,bool required,bool hasVal,s32 argi,char type,void *ptr);
static const char *cmdargs_getArgVal(sCmdArgs *a,s32 i);
static sIterator cmdargs_getFreeArgs(sCmdArgs *a);
static bool cmdargs_itHasNext(sIterator *it);
static void *cmdargs_itNext(sIterator *it);

sCmdArgs *cmdargs_create(int argc,const char **argv) {
	sCmdArgs *a = (sCmdArgs*)heap_alloc(sizeof(sCmdArgs));
	a->argc = argc;
	a->argv = argv;
	a->freeArgs = NULL;
	a->destroy = cmdargs_destroy;
	a->parse = cmdargs_parse;
	a->getFreeArgs = cmdargs_getFreeArgs;
	return a;
}

void cmdargs_destroy(sCmdArgs *a) {
	if(a->freeArgs)
		sll_destroy(a->freeArgs,false);
	heap_free(a);
}

static void cmdargs_parse(sCmdArgs *a,const char *fmt,...) {
	va_list ap;
	bool required;
	bool hasVal;
	char type;
	s32 i;
	char *f = (char*)fmt;

	a->freeArgs = sll_create();
	if(a->freeArgs == NULL)
		THROW(OutOfMemoryException);
	for(i = 1; i < a->argc; i++)
		sll_append(a->freeArgs,(void*)i);

	va_start(ap,fmt);
	while(*f) {
		char *begin = f,*end;
		char c;
		required = false;
		hasVal = false;
		type = '\0';
		do {
			c = *f++;
		}
		while(c && c != '=' && c != ' ' && c != '*');
		end = --f;

		if(*f == '=') {
			hasVal = true;
			f++;
			while(*f && *f != ' ') {
				switch(*f) {
					case 's':
					case 'd':
					case 'i':
					case 'x':
					case 'X':
						type = *f;
						break;
					case '*':
						required = true;
						break;
				}
				f++;
			}
		}

		i = cmdargs_find(a,begin,end);
		cmdargs_setVal(a,required,hasVal,i,type,va_arg(ap,void*));

		while(*f && *f++ != ' ');
	}
	va_end(ap);
}

static s32 cmdargs_find(sCmdArgs *a,char *begin,char *end) {
	s32 i;
	bool onechar = end - begin == 1;
	for(i = 1; i < a->argc; i++) {
		if(onechar) {
			char next = a->argv[i][2];
			if(a->argv[i][0] == '-' && a->argv[i][1] == *begin && (next == '\0' || next == '='))
				return i;
		}
		else {
			char next = a->argv[i][2 + end - begin];
			if(strncmp(a->argv[i],"--",2) == 0 &&
				strncmp(a->argv[i] + 2,begin,end - begin) == 0 && (next == '\0' || next == '='))
				return i;
		}
	}
	return -1;
}

static void cmdargs_setVal(sCmdArgs *a,bool required,bool hasVal,s32 argi,char type,void *ptr) {
	if(required && argi == -1)
		THROW(CmdArgsException);
	if(hasVal) {
		if(argi == -1)
			return;
		switch(type) {
			case 's': {
				const char **str = (const char**)ptr;
				*str = cmdargs_getArgVal(a,argi);
			}
			break;

			case 'd':
			case 'i': {
				s32 *n = (s32*)ptr;
				*n = strtol(cmdargs_getArgVal(a,argi),NULL,10);
			}
			break;

			case 'x':
			case 'X': {
				u32 *u = (u32*)ptr;
				*u = strtoul(cmdargs_getArgVal(a,argi),NULL,16);
			}
			break;
		}
	}
	else {
		bool *b = (bool*)ptr;
		*b = argi != -1;
		if(argi != -1)
			sll_removeFirst(a->freeArgs,(void*)argi);
	}
}

static const char *cmdargs_getArgVal(sCmdArgs *a,s32 i) {
	const char *arg = a->argv[i];
	char *eqPos = strchr(arg,'=');
	sll_removeFirst(a->freeArgs,(void*)i);
	if(eqPos == NULL) {
		if(i >= a->argc - 1)
			THROW(CmdArgsException);
		sll_removeFirst(a->freeArgs,(void*)(i + 1));
		return a->argv[i + 1];
	}
	return eqPos + 1;
}

static sIterator cmdargs_getFreeArgs(sCmdArgs *a) {
	sIterator it;
	vassert(a->freeArgs != NULL,"Please call parse() first!");
	it.con = a;
	it.pos = 0;
	it.next = cmdargs_itNext;
	it.hasNext = cmdargs_itHasNext;
	return it;
}

static bool cmdargs_itHasNext(sIterator *it) {
	sCmdArgs *a = (sCmdArgs*)it->con;
	return it->pos < sll_length(a->freeArgs);
}

static void *cmdargs_itNext(sIterator *it) {
	sCmdArgs *a = (sCmdArgs*)it->con;
	s32 i;
	if(it->pos >= sll_length(a->freeArgs))
		return NULL;
	i = (s32)sll_get(a->freeArgs,it->pos);
	it->pos++;
	return (void*)a->argv[i];
}
