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

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include <assert.h>
#include "symbols.h"

struct sFuncCall {
	sFuncCall *parent;
	sFuncCall *next;
	sFuncCall *child;
	char name[MAX_FUNC_LEN + 1];
	unsigned long long addr;
	unsigned long long time;
	unsigned long long begin;
	unsigned long calls;
	int running;
};

struct sContext {
	unsigned long layer;
	sFuncCall *current;
	sFuncCall *root;
};

typedef void (*fParse)(FILE *f);
typedef struct {
	const char *name;
	fParse parse;
} sParser;

static sContext *getCurrent(unsigned long tid);
static void funcEnter(unsigned long tid,const char *name,unsigned long long addr);
static void funcLeave(unsigned long tid,unsigned long long time);
static void parseI586(FILE *f);
static void parseMMIX(FILE *f);
static const char *resolve(const char *name,unsigned long long addr);
static sFuncCall *getFunc(sFuncCall *cur,const char *name,unsigned long long addr);
static sFuncCall *append(sFuncCall *cur,const char *name,unsigned long long addr);
static unsigned long long leaveFuncs(sFuncCall *f);
static void printFunc(sFuncCall *f,int layer);

static sParser parsers[] = {
	{"i586",parseI586},
	{"mmix",parseMMIX},
};
static unsigned long contextSize = 0;
static sContext *contexts;

int main(int argc,char *argv[]) {
	unsigned long long totalTime;
	unsigned long tid;
	bool haveFile = false;
	FILE *f = stdin;
	int parser = -1;

	if(argc < 3) {
		fprintf(stderr,"Usage: %s <format> <input> [<symbolFile>...]\n",argv[0]);
		return EXIT_FAILURE;
	}

	for(size_t i = 0; i < sizeof(parsers) / sizeof(parsers[0]); i++) {
		if(strcmp(argv[1],parsers[i].name) == 0) {
			parser = i;
			break;
		}
	}
	if(parser == -1) {
		fprintf(stderr,"'%s' is no known format. Use 'i586' or 'mmix'.\n",argv[1]);
		return EXIT_FAILURE;
	}

	if(strcmp(argv[2],"-") != 0) {
		haveFile = true;
		f = fopen(argv[2],"r");
		if(!f)
			perror("fopen");
	}

	sym_init();
	for(int i = 3; i < argc; i++)
		sym_addFile(argv[i]);

	parsers[parser].parse(f);
	if(haveFile)
		fclose(f);

	/* calculate total time and leave all functions that have not been left so far (e.g. main) */
	totalTime = 0;
	for(tid = 0; tid < contextSize; tid++) {
		if(contexts[tid].current) {
			leaveFuncs(contexts[tid].root);
			totalTime += contexts[tid].root->time;
		}
	}
	/* print header */
	printf("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
	printf("<functionCalls>\n");
	printf("  <fileName>dummyFile</fileName>\n");
	printf("  <totalTime>%Lu</totalTime>\n",totalTime);
	printf("  <totalMem>0</totalMem>\n");
	for(tid = 0; tid < contextSize; tid++) {
		if(contexts[tid].current)
			printFunc(contexts[tid].root,1);
	}
	printf("</functionCalls>\n");

	return EXIT_SUCCESS;
}

static void parseI586(FILE *f) {
	char funcName[MAX_FUNC_LEN + 1];
	int c;
	unsigned long tid;
	unsigned long long time;
	while((c = getc(f)) != EOF) {
		/* function-enter */
		if(c == '>') {
			fscanf(f,"%lu:",&tid);
			fscanf(f,"%s",funcName);
			funcEnter(tid,funcName,0);
		}
		/* function-return */
		else if(c == '<') {
			fscanf(f,"%lu:",&tid);
			fscanf(f,"%Lu",&time);
			funcLeave(tid,time);
		}
	}
}

static void parseMMIX(FILE *f) {
	char funcName[MAX_FUNC_LEN + 1];
	int c,tid;
	unsigned long long addr,time;
	long line = 1;
	while((c = getc(f)) != EOF) {
		sContext *con;
		/* skip whitespace at the beginning */
		while(isspace(c))
			c = getc(f);
		/* function-enter */
		if(c == '\\') {
			size_t i;
			/* skip -- */
			while(getc(f) == '-')
				;
			for(i = 0; i < MAX_FUNC_LEN && (c = getc(f)) != '('; i++) {
				funcName[i] = c;
				if(c == '>') {
					i++;
					break;
				}
			}
			funcName[i] = '\0';
			while((c = getc(f)) != '>')
				;
			assert(fscanf(f," #%Lx], t=%d, ic=#%Lx",&addr,&tid,&time) == 3);
			funcEnter(tid,funcName,addr);
			con = getCurrent(tid);
			con->current->begin = time;
		}
		/* function-return */
		else if(c == '/') {
			/* skip -- */
			while(getc(f) == '-')
				;
			con = getCurrent(tid);
			assert(fscanf(f," t=%d, ic=#%Lx",&tid,&time) == 2);
			funcLeave(tid,con->current->begin < time ?
					time - con->current->begin : con->current->begin);
		}
		/* to line end */
		while(getc(f) != '\n')
			;
		line++;
	}
}

static sContext *getCurrent(unsigned long tid) {
	if(tid >= contextSize) {
		unsigned long oldSize = contextSize;
		contextSize = contextSize == 0 ? std::max(8UL,tid + 1) : std::max(contextSize * 2,tid + 1);
		contexts = (sContext*)realloc(contexts,contextSize * sizeof(sContext));
		memset(contexts + oldSize,0,(contextSize - oldSize) * sizeof(sContext));
	}
	if(contexts[tid].current == NULL) {
		contexts[tid].layer = 0;
		contexts[tid].root = (sFuncCall*)malloc(sizeof(sFuncCall));
		contexts[tid].root->next = NULL;
		contexts[tid].root->child = NULL;
		contexts[tid].root->parent = NULL;
		snprintf(contexts[tid].root->name,sizeof(contexts[tid].root->name),"Thread %lu",tid);
		contexts[tid].root->time = 0;
		contexts[tid].root->calls = 0;
		contexts[tid].root->running = 1;
		contexts[tid].current = contexts[tid].root;
	}
	return contexts + tid;
}

static void funcEnter(unsigned long tid,const char *name,unsigned long long addr) {
	sContext *context = getCurrent(tid);
	sFuncCall *call = getFunc(context->current,name,addr);
	if(call == NULL)
		context->current = append(context->current,name,addr);
	else
		context->current = call;
	context->current->running = 1;
	context->current->calls++;
	context->layer++;
}

static void funcLeave(unsigned long tid,unsigned long long time) {
	sContext *context = getCurrent(tid);
	if(context->layer > 0) {
		context->current->running = 0;
		context->current->time += time;
		context->current = context->current->parent;
		context->layer--;
	}
}

static const char *resolve(const char *name,unsigned long long addr) {
	static char resolved[MAX_FUNC_LEN] = "";
	unsigned long long naddr;
	naddr = strtoull(name,NULL,16);
	specialChars(name,resolved,MAX_FUNC_LEN);
	if(std::string(name).find_first_not_of("0123456789ABCDEFabcdef",0) == std::string::npos) {
		const char *symname = sym_resolve(naddr);
		strncat(resolved,": ",std::min((size_t)2,MAX_FUNC_LEN - strlen(resolved)));
		strncat(resolved,symname,std::min(strlen(symname),MAX_FUNC_LEN - strlen(resolved)));
	}
	else if(addr != 0)
		snprintf(resolved + strlen(resolved),MAX_FUNC_LEN - strlen(resolved),": #%Lx",addr);
	return resolved;
}

static sFuncCall *getFunc(sFuncCall *cur,const char *name,unsigned long long addr) {
	if(!cur->child)
		return NULL;
	const char *rname = resolve(name,addr);
	sFuncCall *c = cur->child;
	while(c != NULL) {
		if(strcmp(c->name,rname) == 0)
			return c;
		c = c->next;
	}
	return NULL;
}

static sFuncCall *append(sFuncCall *cur,const char *name,unsigned long long addr) {
	sFuncCall *c;
	sFuncCall *call = (sFuncCall*)malloc(sizeof(sFuncCall));
	if(call == NULL)
		exit(EXIT_FAILURE);
	call->calls = 0;
	call->time = 0;
	call->parent = cur;
	call->child = NULL;
	call->next = NULL;
	call->addr = addr;
	call->running = 0;
	strncpy(call->name,resolve(name,addr),MAX_FUNC_LEN);

	c = cur->child;
	if(c) {
		while(c->next != NULL)
			c = c->next;
		c->next = call;
	}
	else
		cur->child = call;
	return call;
}

static unsigned long long leaveFuncs(sFuncCall *f) {
	unsigned long long res = f->time;
	if(f->running) {
		/* note that we losing the self-time here, because we're only summing up the sub-calls
		 * and pretending that the function itself has not used any time at all */
		sFuncCall *c = f->child;
		while(c != NULL) {
			f->time += leaveFuncs(c);
			c = c->next;
		}
		f->running = 0;
		res = f->time;
	}
	return res;
}

static void printFunc(sFuncCall *f,int layer) {
	if(f) {
		sFuncCall *c = f;
		while(c != NULL) {
			printf("%*s<functionCall id=\"%s\">\n",layer * 2," ",c->name);
			printf("%*s  <class></class>\n",layer * 2," ");
			printf("%*s  <function>%s</function>\n",layer * 2," ",c->name);
			printf("%*s  <file>dummyFile</file>\n",layer * 2," ");
			printf("%*s  <line>0</line>\n",layer * 2," ");
			printf("%*s  <mem>0</mem>\n",layer * 2," ");
			printf("%*s  <time>%Lu</time>\n",layer * 2," ",c->time);
			printf("%*s  <calls>%ld</calls>\n",layer * 2," ",c->calls);
			printf("%*s  <subFunctions>\n",layer * 2," ");
			printFunc(c->child,layer + 1);
			printf("%*s  </subFunctions>\n",layer * 2," ");
			printf("%*s</functionCall>\n",layer * 2," ");
			c = c->next;
		}
	}
}
