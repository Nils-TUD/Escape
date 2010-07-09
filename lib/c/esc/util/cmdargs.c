/**
 * $Id: cmdargs.c 646 2010-05-04 15:19:36Z nasmussen $
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
#include <esc/util/cmdargs.h>
#include <esc/exceptions/outofmemory.h>
#include <esc/exceptions/cmdargs.h>
#include <assert.h>
#include <esc/sllist.h>
#include <string.h>
#include <ctype.h>

#define MAX_ARGNAME_LEN		16

static void cmdargs_destroy(sCmdArgs *a);
static const char *cmdargs_getFirstFree(sCmdArgs *a);
static void cmdargs_parse(sCmdArgs *a,const char *fmt,...);
static s32 cmdargs_find(sCmdArgs *a,const char *begin,const char *end,bool hasVal);
static void cmdargs_setVal(sCmdArgs *a,bool hasVal,bool isEmpty,const char *begin,
		s32 argi,char type,void *ptr);
static u32 cmdargs_readk(const char *str);
static const char *cmdargs_getArgVal(sCmdArgs *a,s32 i,bool isEmpty,bool hasVal,const char *begin);
static sIterator cmdargs_getFreeArgs(sCmdArgs *a);
static bool cmdargs_itHasNext(sIterator *it);
static void *cmdargs_itNext(sIterator *it);

sCmdArgs *cmdargs_create(int argc,const char **argv,u16 flags) {
	sCmdArgs *a = (sCmdArgs*)heap_alloc(sizeof(sCmdArgs));
	a->_argc = argc;
	a->_argv = argv;
	a->_flags = flags;
	a->_freeArgs = NULL;
	a->isHelp = false;
	a->destroy = cmdargs_destroy;
	a->parse = cmdargs_parse;
	a->getFirstFree = cmdargs_getFirstFree;
	a->getFreeArgs = cmdargs_getFreeArgs;
	return a;
}

static void cmdargs_destroy(sCmdArgs *a) {
	if(a->_freeArgs)
		sll_destroy(a->_freeArgs,false);
	heap_free(a);
}

static const char *cmdargs_getFirstFree(sCmdArgs *a) {
	if(sll_length(a->_freeArgs) > 0) {
		s32 i = (s32)sll_get(a->_freeArgs,0);
		return a->_argv[i];
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
	a->_freeArgs = sll_create();
	if(a->_freeArgs == NULL)
		THROW(OutOfMemoryException);
	for(i = 1; i < a->_argc; i++)
		sll_append(a->_freeArgs,(void*)i);

	/* check for help-requests (default flags here) */
	flagsBak = a->_flags;
	a->_flags = 0;
	for(i = 0; i < (s32)ARRAY_SIZE(helps); i++) {
		s32 index = cmdargs_find(a,helps[i],helps[i] + strlen(helps[i]),true);
		if(index != -1) {
			a->isHelp = true;
			return;
		}
	}
	a->_flags = flagsBak;

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
		i = cmdargs_find(a,begin,end,hasVal);
		if(required && i == -1) {
			char argName[MAX_ARGNAME_LEN];
			strncpy(argName,begin,end - begin);
			argName[end - begin] = '\0';
			THROW(CmdArgsException,"Required argument '%s' is missing",argName);
		}
		cmdargs_setVal(a,hasVal,begin == end,begin,i,type,va_arg(ap,void*));

		/* to next */
		while(*f && *f++ != ' ');
	}
	va_end(ap);

	/* check free args */
	if((a->_flags & CA_NO_FREE) && sll_length(a->_freeArgs) > 0)
		THROW(CmdArgsException,"Free arguments are not allowed");
	if((a->_flags & CA_MAX1_FREE) && sll_length(a->_freeArgs) > 1)
		THROW(CmdArgsException,"Max. 1 free argument is allowed");
}

static s32 cmdargs_find(sCmdArgs *a,const char *begin,const char *end,bool hasVal) {
	s32 i;
	bool onechar = end - begin == 1;
	/* empty? */
	if(begin == end) {
		/* use the first free arg */
		if(sll_length(a->_freeArgs) == 0)
			THROW(CmdArgsException,"Required argument is missing");
		return (s32)sll_get(a->_freeArgs,0);
	}
	for(i = 1; i < a->_argc; i++) {
		const char *next;
		if(onechar) {
			next = (a->_flags & CA_NO_DASHES) ? &a->_argv[i][1] : &a->_argv[i][2];
			if(!(a->_flags & CA_NO_DASHES) && a->_argv[i][0] != '-')
				continue;
			/* multiple flags may be passed with one argument */
			if(!hasVal && strchr(next - 1,*begin) != NULL)
				return i;
			/* otherwise the first char has to match and has to be the last, too */
			if(next[-1] != *begin)
				continue;
		}
		else {
			next = (a->_flags & CA_NO_DASHES) ? &a->_argv[i][end - begin] : &a->_argv[i][2 + end - begin];
			if(a->_flags & CA_NO_DASHES) {
				if(strncmp(a->_argv[i],begin,end - begin))
					continue;
			}
			else {
				if(strncmp(a->_argv[i],"--",2) || strncmp(a->_argv[i] + 2,begin,end - begin))
					continue;
			}
		}
		if(!(a->_flags & CA_NO_EQ) && *next == '=')
			return i;
		if(!(a->_flags & CA_REQ_EQ) && *next == '\0')
			return i;
	}
	return -1;
}

static void cmdargs_setVal(sCmdArgs *a,bool hasVal,bool isEmpty,const char *begin,
		s32 argi,char type,void *ptr) {
	if(hasVal) {
		if(argi == -1)
			return;
		switch(type) {
			case 's': {
				const char **str = (const char**)ptr;
				*str = cmdargs_getArgVal(a,argi,isEmpty,hasVal,begin);
			}
			break;

			case 'c': {
				char *c = (char*)ptr;
				const char *str = cmdargs_getArgVal(a,argi,isEmpty,hasVal,begin);
				*c = *str;
			}
			break;

			case 'd':
			case 'i': {
				s32 *n = (s32*)ptr;
				*n = strtol(cmdargs_getArgVal(a,argi,isEmpty,hasVal,begin),NULL,10);
			}
			break;

			case 'x':
			case 'X': {
				u32 *u = (u32*)ptr;
				*u = strtoul(cmdargs_getArgVal(a,argi,isEmpty,hasVal,begin),NULL,16);
			}
			break;

			case 'k': {
				u32 *k = (u32*)ptr;
				*k = cmdargs_readk(cmdargs_getArgVal(a,argi,isEmpty,hasVal,begin));
			}
			break;
		}
	}
	else {
		bool *b = (bool*)ptr;
		*b = argi != -1;
		if(argi != -1)
			sll_removeFirst(a->_freeArgs,(void*)argi);
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

static const char *cmdargs_getArgVal(sCmdArgs *a,s32 i,bool isEmpty,bool hasVal,const char *begin) {
	const char *arg = a->_argv[i];
	char *eqPos = strchr(arg,'=');
	sll_removeFirst(a->_freeArgs,(void*)i);
	if(eqPos == NULL) {
		/* if its a flag, simply return the pointer to the character; we'll just use the first one
		 * anyway */
		if(!hasVal)
			return strchr(a->_argv[i],*begin);
		if(!isEmpty) {
			if((a->_flags & CA_REQ_EQ) || i >= a->_argc - 1)
				THROW(CmdArgsException,"Please use '=' to specify values");
			sll_removeFirst(a->_freeArgs,(void*)(i + 1));
			return a->_argv[i + 1];
		}
		return a->_argv[i];
	}
	if(!hasVal)
		THROW(CmdArgsException,"Found '=' in flag-argument");
	if(a->_flags & CA_NO_EQ)
		THROW(CmdArgsException,"Please use no '=' to specify values");
	return eqPos + 1;
}

static sIterator cmdargs_getFreeArgs(sCmdArgs *a) {
	sIterator it;
	vassert(a->_freeArgs != NULL,"Please call parse() first!");
	it._con = a;
	it._pos = 0;
	it._end = 0;
	it.next = cmdargs_itNext;
	it.hasNext = cmdargs_itHasNext;
	return it;
}

static bool cmdargs_itHasNext(sIterator *it) {
	sCmdArgs *a = (sCmdArgs*)it->_con;
	return it->_pos < sll_length(a->_freeArgs);
}

static void *cmdargs_itNext(sIterator *it) {
	sCmdArgs *a = (sCmdArgs*)it->_con;
	s32 i;
	if(it->_pos >= sll_length(a->_freeArgs))
		return NULL;
	i = (s32)sll_get(a->_freeArgs,it->_pos);
	it->_pos++;
	return (void*)a->_argv[i];
}
