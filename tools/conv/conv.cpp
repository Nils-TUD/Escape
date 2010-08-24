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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbols.h"

struct sFuncCall {
	sFuncCall *parent;
	sFuncCall *next;
	sFuncCall *child;
	char name[MAX_FUNC_LEN + 1];
	unsigned long long time;
	unsigned long calls;
};

static const char *resolve(const char *name) {
	static char resolved[MAX_FUNC_LEN];
	unsigned long addr;
	char *end;
	addr = strtoul(name,&end,16);
	strcpy(resolved,name);
	if(addr) {
		strcat(resolved,": ");
		strcat(resolved,sym_resolve(addr));
	}
	return resolved;
}

static sFuncCall *getFunc(sFuncCall *cur,const char *name) {
	if(!cur->child)
		return NULL;
	const char *rname = resolve(name);
	sFuncCall *c = cur->child;
	while(c != NULL) {
		if(strcmp(c->name,rname) == 0)
			return c;
		c = c->next;
	}
	return NULL;
}

static sFuncCall *append(sFuncCall *cur,const char *name) {
	sFuncCall *c;
	sFuncCall *call = (sFuncCall*)malloc(sizeof(sFuncCall));
	if(call == NULL)
		exit(EXIT_FAILURE);
	call->calls = 0;
	call->time = 0;
	call->parent = cur;
	call->child = NULL;
	call->next = NULL;
	strcpy(call->name,resolve(name));

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

static void printFunc(sFuncCall *f,int layer) {
	if(f) {
		sFuncCall *c = f->child;
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
			printFunc(c,layer + 1);
			printf("%*s  </subFunctions>\n",layer * 2," ");
			printf("%*s</functionCall>\n",layer * 2," ");
			c = c->next;
		}
	}
}

static char *getSymName(unsigned long addr) {
	/*readelf -sW build/debug/user_cppsort.bin | grep 2fff | xargs | cut -d ' ' -f 8 | c++filt*/
}

int main(int argc,char *argv[]) {
	char funcName[MAX_FUNC_LEN + 1];
	unsigned long long totalTime;
	unsigned long long time;
	char c;
	int layer;
	sFuncCall *cur;
	sFuncCall root;
	root.next = NULL;
	root.child = NULL;
	root.parent = NULL;
	strcpy(root.name,"main");
	root.time = 0;
	root.calls = 0;
	FILE *f = stdin;

	if(strcmp(argv[1],"-f") == 0) {
		f = fopen(argv[2],"r");
		if(!f)
			perror("fopen");
		sym_init(argc > 3 ? argv[3] : NULL);
	}
	else
		sym_init(argc > 1 ? argv[1] : NULL);

	cur = &root;
	layer = 0;
	while((c = getc(f)) != EOF) {
		sFuncCall *call;
		/* function-enter */
		if(c == '>') {
			fscanf(f,"%s",funcName);
			call = getFunc(cur,funcName);
			if(call == NULL)
				cur = append(cur,funcName);
			else
				cur = call;
			cur->calls++;
			layer++;
		}
		/* function-return */
		else if(c == '<' && layer > 0) {
			fscanf(f,"%Lu",&time);
			cur->time += time;
			cur = cur->parent;
			layer--;
		}
	}

	if(strcmp(argv[1],"-f") == 0)
		fclose(f);

	/* calculate total time via sum of the root-child-times */
	totalTime = 0;
	cur = root.child;
	while(cur != NULL) {
		totalTime += cur->time;
		cur = cur->next;
	}
	/* print header */
	printf("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
	printf("<functionCalls>\n");
	printf("  <fileName>dummyFile</fileName>\n");
	printf("  <totalTime>%Lu</totalTime>\n",totalTime);
	printf("  <totalMem>0</totalMem>\n");
	printFunc(&root,0);
	printf("</functionCalls>\n");

	return EXIT_SUCCESS;
}
