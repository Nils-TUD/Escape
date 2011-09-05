/**
 * $Id$
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "mmix/io.h"
#include "mmix/mem.h"
#include "cli/lang/bt.h"
#include "cli/lang/symmap.h"

#define MAX_IDENT_LEN 255

static traceFunctionNode* traceFunctionList;

void symmap_append(traceFunctionNode* node) {
	if(traceFunctionList != NULL) {
		traceFunctionList->prev = node;
		node->next = traceFunctionList;
	}
	traceFunctionList = node;
}

const char *symmap_getFuncName(octa address) {
	traceFunctionNode* nodebuffer;
	nodebuffer = traceFunctionList;
	while(nodebuffer != NULL && nodebuffer->address != address) {
		nodebuffer = nodebuffer->next;
	}
	if(nodebuffer == NULL) {
		return "<Unnamed>";
	}
	return nodebuffer->name;
}

octa symmap_getAddress(const char *name) {
	traceFunctionNode* nodebuffer;
	nodebuffer = traceFunctionList;
	while(nodebuffer != NULL) {
		if(strcmp(nodebuffer->name,name) == 0)
			return nodebuffer->address;
		nodebuffer = nodebuffer->next;
	}
	return 0;
}

bool symmap_parse(const char *mapName) {
	FILE *mapfile;
	enum {
		NAME = 0, SECTION = 1, ADDRESS = 2, END = 3, FAIL = 4
	} reading;
	char buffer[MAX_IDENT_LEN];
	int buff_i;
	octa secStart;
	traceFunctionNode template;
	traceFunctionNode *cpyNode;
	char *tmp;
	char currSection;
	char t;
	bool inSegments;
	bool wordready;
	bool lineready;
	traceFunctionNode *start;

	/* open mapfile for reading*/
	mapfile = fopen(mapName,"r");
	if(mapfile == NULL) {
		mprintf("Error: cannot open map-file '%s'. Tracing will be disabled.\n",mapName);
		return false;
	}
	/*initialize data*/
	inSegments = false;
	wordready = false;
	lineready = false;
	reading = NAME;
	template.next = NULL;
	template.prev = NULL;
	template.address = 0;
	template.name = "";
	currSection = 'B';
	buff_i = 0;
	start = traceFunctionList;

	/* get a char*/
	t = getc(mapfile);
	/* loop for parsing */
	while(1) {
		/* if wordready, save word and reset for next one */
		if(wordready) {
			/* start of segments? */
			if(reading == NAME && strncmp(buffer,"CODE",4) == 0) {
				inSegments = true;
				break;
			}
			switch(reading) {
				case NAME:
					tmp = (char*)mem_alloc(sizeof(char) * (buff_i + 1));
					strncpy(tmp,buffer,buff_i);
					tmp[buff_i] = '\0';
					template.name = tmp;
					reading = SECTION;
					break;
				case SECTION:
					currSection = buffer[0];
					reading = ADDRESS;
					break;
				case ADDRESS:
					msscanf(buffer,"0x%016OX",&template.address);
					reading = END;
					break;
				default:
					reading = FAIL;
					break;
			}
			buff_i = 0;
			wordready = false;
			continue;
		}
		/* if lineready, try to generate a struct */
		if(lineready) {
			/* didnt reached the expected end? -> invalid file/break */
			if(reading != END) {
				break;
			}
			/* check if the entry is out of the code section = function */
			if(currSection == 'C') {
				cpyNode = (traceFunctionNode*)mem_alloc(sizeof(traceFunctionNode));
				memcpy(cpyNode,&template,sizeof(traceFunctionNode));
				symmap_append(cpyNode);
			}

			/* reset parser for next line*/
			buff_i = 0;
			lineready = false;
			reading = NAME;
			continue;
		}

		/* EOF? should never be, because map file has invalid "function" lines befor.
		 * so if we reach here, just break.
		 */
		if(t == EOF) {
			break;
		}
		/* newline = wordready,lineready, normaly struct ready. */
		if(t == '\n') {
			buffer[buff_i] = '\0';
			wordready = buff_i > 0;
			lineready = buff_i > 0;
			t = getc(mapfile);
			continue;
		}
		/* ignore spaces but if they occur the first time: wordready */
		if(isspace(t)) {
			buffer[buff_i] = '\0';
			wordready = true;
			while(isspace(t = getc(mapfile)))
				;
			continue;
		}
		buffer[buff_i++] = t;
		/* get next char */
		t = getc(mapfile);
	}

	/* now look for "CODE start 0x...." */
	if(inSegments) {
		/* read "CODE" */
		mfscanf(mapfile,"tart	0x%016OX",&secStart);

		/* change addresses */
		for(cpyNode = traceFunctionList; cpyNode != start; cpyNode = cpyNode->next) {
			cpyNode->address += secStart;
		}
	}
	return true;
}
