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
#include <ctype.h>

static const char *cmdargs_getFirstFree(sCmdArgs *a);
static void cmdargs_parse(sCmdArgs *a,const char *fmt,...);
static s32 cmdargs_find(sCmdArgs *a,const char *begin,const char *end);
static void cmdargs_setVal(sCmdArgs *a,bool hasVal,bool isEmpty,s32 argi,char type,void *ptr);
static u32 cmdargs_readk(const char *str);
static const char *cmdargs_getArgVal(sCmdArgs *a,s32 i,bool isEmpty);
static sIterator cmdargs_getFreeArgs(sCmdArgs *a);
static bool cmdargs_itHasNext(sIterator *it);
static void *cmdargs_itNext(sIterator *it);

sCmdArgs *cmdargs_create(int argc,const char **argv,u16 flags) {
	sCmdArgs *a = (sCmdArgs*)heap_alloc(sizeof(sCmdArgs));
	a->argc = argc;
	a->argv = argv;
	a->flags = flags;
	a->isHelp = false;
	a->freeArgs = NULL;
	a->destroy = cmdargs_destroy;
	a->parse = cmdargs_parse;
	a->getFirstFree = cmdargs_getFirstFree;
	a->getFreeArgs = cmdargs_getFreeArgs;
	return a;
}

void cmdargs_destroy(sCmdArgs *a) {
	if(a->freeArgs)
		sll_destroy(a->freeArgs,false);
	heap_free(a);
}

static const char *cmdargs_getFirstFree(sCmdArgs *a) {
	if(sll_length(a->freeArgs) > 0) {
		s32 i = (s32)sll_get(a->freeArgs,0);
		return a->argv[i];
	}
	return NULL;
}

static void cmdargs_parse(sCmdArgs *a,const char *fmt,...) {
	const char *helps[] = {"h","help","?"};
	va_list ap;
	bool required;
	bool hasVal;
	char type;
	u16 flagsBak;
	s32 i;
	char *f = (char*)fmt;

	/* create free args list */
	a->freeArgs = sll_create();
	if(a->freeArgs == NULL)
		THROW(OutOfMemoryException);
	for(i = 1; i < a->argc; i++)
		sll_append(a->freeArgs,(void*)i);

	/* check for help-requests (default flags here) */
	flagsBak = a->flags;
	a->flags = 0;
	for(i = 0; i < (s32)ARRAY_SIZE(helps); i++) {
		s32 index = cmdargs_find(a,helps[i],helps[i] + strlen(helps[i]));
		if(index != -1) {
			a->isHelp = true;
			return;
		}
	}
	a->flags = flagsBak;

	/* loop through the argument-specification and set the variables to the given arguments */
	va_start(ap,fmt);
	while(*f) {
		char *begin = f,*end;
		char c;
		required = false;
		hasVal = false;
		type = '\0';
		/* first read the name */
		do {
			c = *f++;
		}
		while(c && c != '=' && c != ' ' && c != '*');
		end = --f;

		/* has it a value? */
		if(*f == '=') {
			hasVal = true;
			f++;
			/* read val-type and if its required */
			while(*f && *f != ' ') {
				switch(*f) {
					case 's':
					case 'd':
					case 'i':
					case 'x':
					case 'X':
					case 'k':
					case 'c':
						type = *f;
						break;
					case '*':
						required = true;
						break;
				}
				f++;
			}
		}

		/* find the argument and set it */
		i = cmdargs_find(a,begin,end);
		if(required && i == -1) {
			/* TODO actually we can't write to it since it may be in readonly memory */
			*end = '\0';
			THROW(CmdArgsException,"Required argument '%s' is missing",begin);
		}
		cmdargs_setVal(a,hasVal,begin == end,i,type,va_arg(ap,void*));

		/* to next */
		while(*f && *f++ != ' ');
	}
	va_end(ap);

	/* check free args */
	if((a->flags & CA_NO_FREE) && sll_length(a->freeArgs) > 0)
		THROW(CmdArgsException,"Free arguments are not allowed");
	if((a->flags & CA_MAX1_FREE) && sll_length(a->freeArgs) > 1)
		THROW(CmdArgsException,"Max. 1 free argument is allowed");
}

static s32 cmdargs_find(sCmdArgs *a,const char *begin,const char *end) {
	s32 i;
	bool onechar = end - begin == 1;
	/* empty? */
	if(begin == end) {
		/* use the first free arg */
		if(sll_length(a->freeArgs) == 0)
			THROW(CmdArgsException,"Required argument is missing");
		return (s32)sll_get(a->freeArgs,0);
	}
	for(i = 1; i < a->argc; i++) {
		char next;
		if(onechar) {
			next = (a->flags & CA_NO_DASHES) ? a->argv[i][1] : a->argv[i][2];
			if(a->flags & CA_NO_DASHES) {
				if(a->argv[i][0] != *begin)
					continue;
			}
			else {
				if(a->argv[i][0] != '-' || a->argv[i][1] != *begin)
					continue;
			}
		}
		else {
			next = (a->flags & CA_NO_DASHES) ? a->argv[i][end - begin] : a->argv[i][2 + end - begin];
			if(a->flags & CA_NO_DASHES) {
				if(strncmp(a->argv[i],begin,end - begin))
					continue;
			}
			else {
				if(strncmp(a->argv[i],"--",2) || strncmp(a->argv[i] + 2,begin,end - begin))
					continue;
			}
		}
		if(!(a->flags & CA_NO_EQ) && next == '=')
			return i;
		if(!(a->flags & CA_REQ_EQ) && next == '\0')
			return i;
	}
	return -1;
}

static void cmdargs_setVal(sCmdArgs *a,bool hasVal,bool isEmpty,s32 argi,char type,void *ptr) {
	if(hasVal) {
		if(argi == -1)
			return;
		switch(type) {
			case 's': {
				const char **str = (const char**)ptr;
				*str = cmdargs_getArgVal(a,argi,isEmpty);
			}
			break;

			case 'c': {
				char *c = (char*)ptr;
				const char *str = cmdargs_getArgVal(a,argi,isEmpty);
				*c = *str;
			}
			break;

			case 'd':
			case 'i': {
				s32 *n = (s32*)ptr;
				*n = strtol(cmdargs_getArgVal(a,argi,isEmpty),NULL,10);
			}
			break;

			case 'x':
			case 'X': {
				u32 *u = (u32*)ptr;
				*u = strtoul(cmdargs_getArgVal(a,argi,isEmpty),NULL,16);
			}
			break;

			case 'k': {
				u32 *k = (u32*)ptr;
				*k = cmdargs_readk(cmdargs_getArgVal(a,argi,isEmpty));
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

static u32 cmdargs_readk(const char *str) {
	u32 val = 0;
	while(isdigit(*str))
		val = val * 10 + (*str++ - '0');
	switch(*str) {
		case 'K':
		case 'k':
			val *= 1024;
			break;
		case 'M':
		case 'm':
			val *= 1024 * 1024;
			break;
		case 'G':
		case 'g':
			val *= 1024 * 1024 * 1024;
			break;
	}
	return val;
}

static const char *cmdargs_getArgVal(sCmdArgs *a,s32 i,bool isEmpty) {
	const char *arg = a->argv[i];
	char *eqPos = strchr(arg,'=');
	sll_removeFirst(a->freeArgs,(void*)i);
	if(eqPos == NULL) {
		if(!isEmpty) {
			if((a->flags & CA_REQ_EQ) || i >= a->argc - 1)
				THROW(CmdArgsException,"Please use '=' to specify values");
			sll_removeFirst(a->freeArgs,(void*)(i + 1));
			return a->argv[i + 1];
		}
		return a->argv[i];
	}
	if(a->flags & CA_NO_EQ)
		THROW(CmdArgsException,"Please use no '=' to specify values");
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
