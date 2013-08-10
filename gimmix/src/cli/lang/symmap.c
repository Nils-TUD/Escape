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

/**
 * one entry for the Function list.
 */
typedef struct tFN {
    const char *name;
    octa address;
} traceFunctionNode;

static size_t nodesCount = 0;
static size_t nodesSize = 0;
static traceFunctionNode *nodes;

static int symmap_nodesSort(const void *n1,const void *n2) {
    const traceFunctionNode *node1 = (const traceFunctionNode*)n1;
    const traceFunctionNode *node2 = (const traceFunctionNode*)n2;
    return node1->address > node2->address ? 1 : -1;
}

static bool symmap_append(traceFunctionNode node) {
    if(nodesCount >= nodesSize) {
        nodesSize = MAX(16,nodesSize * 2);
        nodes = realloc(nodes,nodesSize * sizeof(traceFunctionNode));
        if(!nodes)
            return false;
    }
    nodes[nodesCount++] = node;
    return true;
}

const char *symmap_getNearestFuncName(octa address,octa *nearest) {
    long low = 0, high = nodesCount - 1;
    while(low <= high) {
        long i = (low + high) / 2;
        long res = nodes[i].address > address ? 1 : -1;
        if(res == 0) {
            *nearest = nodes[i].address;
            return nodes[i].name;
        }
        if(res < 0) {
            /* if it's in the upper half and the following one is already larger, we use this.
             * thereby, we fall back to the last function-start */
            if((size_t)i < nodesCount - 1 && nodes[i + 1].address > address) {
                *nearest = nodes[i].address;
                return nodes[i].name;
            }
            low = i + 1;
        }
        else
            high = i - 1;
    }
    return "-Unnamed-";
}

const char *symmap_getFuncName(octa address) {
    size_t i;
    for(i = 0; i < nodesCount; ++i) {
        if(nodes[i].address == address)
            return nodes[i].name;
    }
	return "-Unnamed-";
}

octa symmap_getAddress(const char *name) {
    size_t i;
    for(i = 0; i < nodesCount; ++i) {
		if(strcmp(nodes[i].name,name) == 0)
			return nodes[i].address;
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
	char *tmp;
	char currSection;
	char t;
	bool inSegments;
	bool wordready;
	bool lineready;

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
	template.address = 0;
	template.name = "";
	currSection = 'B';
	buff_i = 0;

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
				if(!symmap_append(template)) {
				    mprintf("Error: not enough memory for symbol map.\n");
				    return false;
				}
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
        size_t i;
		/* read "CODE" */
		mfscanf(mapfile,"tart	0x%016OX",&secStart);

		/* change addresses */
	    for(i = 0; i < nodesCount; ++i)
			nodes[i].address += secStart;
	    qsort(nodes,nodesCount,sizeof(traceFunctionNode),symmap_nodesSort);
	}
	return true;
}
