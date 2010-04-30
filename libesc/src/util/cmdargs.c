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
#include <string.h>

static void cmdargs_parse(sCmdArgs *a,const char *fmt,...);
static s32 cmdargs_find(sCmdArgs *a,char *begin,char *end);
static void cmdargs_setVal(sCmdArgs *a,bool required,bool hasVal,s32 argi,char type,void *ptr);
static sIterator cmdargs_getFreeArgs(sCmdArgs *a);
static bool cmdargs_itHasNext(sIterator *it);
static void *cmdargs_itNext(sIterator *it);
static s32 cmdargs_itGetNext(sIterator *it,sCmdArgs *a);

sCmdArgs *cmdargs_create(int argc,char **argv) {
	sCmdArgs *a = (sCmdArgs*)heap_alloc(sizeof(sCmdArgs));
	a->argc = argc;
	a->argv = argv;
	a->destroy = cmdargs_destroy;
	a->parse = cmdargs_parse;
	a->getFreeArgs = cmdargs_getFreeArgs;
	return a;
}

void cmdargs_destroy(sCmdArgs *a) {
	heap_free(a);
}

static void cmdargs_parse(sCmdArgs *a,const char *fmt,...) {
	va_list ap;
	bool required;
	bool hasVal;
	bool onechar;
	char type;
	char *f = (char*)fmt;

	va_start(ap,fmt);
	while(*f) {
		char *begin = f,*end;
		char c;
		do {
			c = *f++;
		}
		while(c != '=' && c != ' ' && c != '*');
		end = f;
		onechar = end - begin == 1;

		if(*f == '=') {
			hasVal = true;
			f++;
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
		}

		cmdargs_setVal(a,required,hasVal,cmdargs_find(a,begin,end),type,va_arg(ap,void*));

		while(*f && *f++ != ' ');
	}
	va_end(ap);
}

static s32 cmdargs_find(sCmdArgs *a,char *begin,char *end) {
	s32 i;
	bool onechar = end - begin == 1;
	for(i = 1; i < a->argc; i++) {
		if(onechar && a->argv[i][0] == '-' && a->argv[i][1] == *begin &&
				a->argv[i][2] == '\0')
			return i;
		if(!onechar && strncmp(a->argv[i],"--",2) == 0 &&
				strncmp(a->argv[i] + 2,begin,end - begin) == 0 &&
				strlen(a->argv[i]) == (u32)(end - begin) + 2)
			return i;
	}
	return -1;
}

static void cmdargs_setVal(sCmdArgs *a,bool required,bool hasVal,s32 argi,char type,void *ptr) {
	/* TODO */
	if((required && argi == -1) || argi >= a->argc - 1)
		/*THROW(CmdArgException);*/error("Cmdargexception");
	if(hasVal) {
		if(argi == -1)
			return;
		switch(type) {
			case 's': {
				char **str = (char**)ptr;
				*str = a->argv[argi + 1];
			}
			break;

			case 'd':
			case 'i': {
				s32 *n = (s32*)ptr;
				*n = strtol(a->argv[argi + 1],NULL,10);
			}
			break;

			case 'x':
			case 'X': {
				u32 *u = (u32*)ptr;
				*u = strtoul(a->argv[argi + 1],NULL,16);
			}
			break;
		}
	}
	else {
		bool *b = (bool*)ptr;
		*b = argi != -1;
	}
}

static sIterator cmdargs_getFreeArgs(sCmdArgs *a) {
	sIterator it;
	it.con = a;
	it.pos = 1;
	it.next = cmdargs_itNext;
	it.hasNext = cmdargs_itHasNext;
	return it;
}

static bool cmdargs_itHasNext(sIterator *it) {
	sCmdArgs *a = (sCmdArgs*)it->con;
	return cmdargs_itGetNext(it,a) != -1;
}

static void *cmdargs_itNext(sIterator *it) {
	sCmdArgs *a = (sCmdArgs*)it->con;
	s32 i = cmdargs_itGetNext(it,a);
	if(i == -1)
		return NULL;
	it->pos = i + 1;
	return a->argv[i];
}

static s32 cmdargs_itGetNext(sIterator *it,sCmdArgs *a) {
	s32 i;
	if((s32)it->pos >= a->argc)
		return -1;
	for(i = it->pos; i < a->argc; i++) {
		if(a->argv[i][0] != '-')
			return i;
	}
	return -1;
}
