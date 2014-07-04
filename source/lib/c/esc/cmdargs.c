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

#include <esc/common.h>
#include <esc/cmdargs.h>
#include <esc/sllist.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#define MAX_ERR_LEN			64
#define MAX_ARGNAME_LEN		16

static ssize_t ca_find(const char *begin,const char *end,bool hasVal);
static ssize_t ca_setVal(bool hasVal,bool isEmpty,const char *begin,ssize_t argi,char type,void *ptr);
static size_t ca_readk(const char *str);
static ssize_t ca_getArgVal(ssize_t i,bool isEmpty,bool hasVal,const char *begin,const char **val);

bool isHelpCmd(int argc,char **argv) {
	if(argc <= 1)
		return false;

	return strcmp(argv[1],"-h") == 0 ||
		strcmp(argv[1],"--help") == 0 ||
		strcmp(argv[1],"-?") == 0;
}

static char errmsg[MAX_ERR_LEN];
static uint flags;
static int argc;
static const char **argv;
static bool hasHelp = false;
static sSLList *freeArgs = NULL;
static const char **freeArgsArray = NULL;
static size_t freeArgCount = 0;

const char *ca_error(int err) {
	switch(err) {
		case CA_ERR_REQUIRED_MISSING:
			return errmsg;
		case CA_ERR_EQ_REQUIRED:
			strcpy(errmsg,"Please use '=' to specify values");
			return errmsg;
		case CA_ERR_EQ_DISALLOWED:
			strcpy(errmsg,"Please use no '=' to specify values");
			return errmsg;
		case CA_ERR_EQ_IN_FLAGARG:
			strcpy(errmsg,"Found '=' in flag-argument");
			return errmsg;
		case CA_ERR_FREE_DISALLOWED:
			strcpy(errmsg,"Free arguments are not allowed");
			return errmsg;
		case CA_ERR_MAX1_FREE:
			strcpy(errmsg,"Max. 1 free argument is allowed");
			return errmsg;
		case CA_ERR_NO_FREE:
			strcpy(errmsg,"Required argument is missing");
			return errmsg;
	}
	return "";
}

bool ca_hasHelp(void) {
	return hasHelp;
}

size_t ca_getFreeCount(void) {
	return freeArgCount;
}

const char **ca_getFree(void) {
	vassert(freeArgsArray != NULL,"Please call ca_parse() first!");
	return freeArgsArray;
}

int ca_parse(int argcnt,const char **args,uint aflags,const char *fmt,...) {
	const char *helps[] = {"-h","--help","-?"};
	va_list ap;
	sSLNode *n;
	bool required;
	bool hasVal;
	char type;
	ssize_t i,j;
	char *f = (char*)fmt;

	flags = aflags;
	argc = argcnt;
	argv = args;

	/* create free args list */
	freeArgs = sll_create();
	if(freeArgs == NULL)
		return -ENOMEM;
	for(i = 1; i < argc; i++)
		sll_append(freeArgs,(void*)i);

	/* check for help-requests */
	for(i = 1; i < argc; i++) {
		for(j = 0; j < (ssize_t)ARRAY_SIZE(helps); j++) {
			if(strcmp(argv[i],helps[j]) == 0) {
				hasHelp = true;
				return 0;
			}
		}
	}

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
		i = ca_find(begin,end,hasVal);
		if(required && i == (ssize_t)CA_ERR_REQUIRED_MISSING) {
			char argName[MAX_ARGNAME_LEN];
			strncpy(argName,begin,end - begin);
			argName[end - begin] = '\0';
			snprintf(errmsg,MAX_ERR_LEN,"Required argument '%s' is missing",argName);
			return CA_ERR_REQUIRED_MISSING;
		}
		if(i != CA_ERR_REQUIRED_MISSING && i < 0)
			return i;
		i = ca_setVal(hasVal,begin == end,begin,i,type,va_arg(ap,void*));
		if(i < 0)
			return i;

		/* to next */
		while(*f && *f++ != ' ');
	}
	va_end(ap);

	/* check free args */
	if((flags & CA_NO_FREE) && sll_length(freeArgs) > 0)
		return CA_ERR_FREE_DISALLOWED;
	if((flags & CA_MAX1_FREE) && sll_length(freeArgs) > 1)
		return CA_ERR_MAX1_FREE;

	/* create free-args-array */
	freeArgsArray = (const char**)malloc((sll_length(freeArgs) + 1) * sizeof(char*));
	if(freeArgsArray == NULL)
		return -ENOMEM;
	for(i = 0, n = sll_begin(freeArgs); n != NULL; n = n->next,i++)
		freeArgsArray[i] = argv[(size_t)n->data];
	freeArgsArray[i] = NULL;
	freeArgCount = i;
	sll_destroy(freeArgs,false);
	return 0;
}

static ssize_t ca_find(const char *begin,const char *end,bool hasVal) {
	ssize_t i;
	bool onechar = end - begin == 1;
	/* empty? */
	if(begin == end) {
		/* use the first free arg */
		if(sll_length(freeArgs) == 0)
			return CA_ERR_NO_FREE;
		return (ssize_t)sll_get(freeArgs,0);
	}
	for(i = 1; i < argc; i++) {
		const char *next;
		if(onechar) {
			next = (flags & CA_NO_DASHES) ? &argv[i][1] : &argv[i][2];
			if(!(flags & CA_NO_DASHES) && argv[i][0] != '-')
				continue;
			/* multiple flags may be passed with one argument */
			if(!hasVal && strchr(next - 1,*begin) != NULL)
				return i;
			/* otherwise the first char has to match and has to be the last, too */
			if(next[-1] != *begin)
				continue;
		}
		else {
			next = (flags & CA_NO_DASHES) ? &argv[i][end - begin] : &argv[i][2 + end - begin];
			if(flags & CA_NO_DASHES) {
				if(strncmp(argv[i],begin,end - begin))
					continue;
			}
			else {
				if(strncmp(argv[i],"--",2) || strncmp(argv[i] + 2,begin,end - begin))
					continue;
			}
		}
		if(!(flags & CA_NO_EQ) && *next == '=')
			return i;
		if(!(flags & CA_REQ_EQ) && *next == '\0')
			return i;
	}
	return CA_ERR_REQUIRED_MISSING;
}

static ssize_t ca_setVal(bool hasVal,bool isEmpty,const char *begin,ssize_t argi,char type,void *ptr) {
	ssize_t res = 0;
	const char *val;
	if(hasVal) {
		if(argi < 0)
			return 0;
		switch(type) {
			case 's': {
				const char **str = (const char**)ptr;
				res = ca_getArgVal(argi,isEmpty,hasVal,begin,str);
			}
			break;

			case 'c': {
				char *c = (char*)ptr;
				res = ca_getArgVal(argi,isEmpty,hasVal,begin,&val);
				if(res == 0)
					*c = *val;
			}
			break;

			case 'd':
			case 'i': {
				int *n = (int*)ptr;
				if((res = ca_getArgVal(argi,isEmpty,hasVal,begin,&val)) == 0)
					*n = strtol(val,NULL,10);
			}
			break;

			case 'x':
			case 'X': {
				uint *u = (uint*)ptr;
				if((res = ca_getArgVal(argi,isEmpty,hasVal,begin,&val)) == 0)
					*u = strtoul(val,NULL,16);
			}
			break;

			case 'k': {
				size_t *k = (size_t*)ptr;
				if((res = ca_getArgVal(argi,isEmpty,hasVal,begin,&val)) == 0)
					*k = ca_readk(val);
			}
			break;
		}
	}
	else {
		int *b = (int*)ptr;
		*b = argi >= 0;
		if(argi >= 0)
			sll_removeFirstWith(freeArgs,(void*)argi);
	}
	return res;
}

static size_t ca_readk(const char *str) {
	size_t val = 0;
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

static ssize_t ca_getArgVal(ssize_t i,bool isEmpty,bool hasVal,const char *begin,const char **val) {
	const char *arg = argv[i];
	char *eqPos = strchr(arg,'=');
	sll_removeFirstWith(freeArgs,(void*)i);
	if(eqPos == NULL) {
		/* if its a flag, simply return the pointer to the character; we'll just use the first one
		 * anyway */
		if(!hasVal) {
			*val = strchr(argv[i],*begin);
			return 0;
		}
		if(!isEmpty) {
			if((flags & CA_REQ_EQ) || i >= argc - 1)
				return CA_ERR_EQ_REQUIRED;
			sll_removeFirstWith(freeArgs,(void*)(i + 1));
			*val = argv[i + 1];
			return 0;
		}
		*val = argv[i];
		return 0;
	}
	if(!hasVal)
		return CA_ERR_EQ_IN_FLAGARG;
	if(flags & CA_NO_EQ)
		return CA_ERR_EQ_DISALLOWED;
	*val = eqPos + 1;
	return 0;
}
