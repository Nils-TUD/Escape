#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common.h"

#include "map.h"

#define MAX_IDENT_LEN 255

/*
 * The List of all known Functions
 */
traceFunctionNode* traceFunctionList;

/*
 * Append an Entry to the traceFunctionList
 */
void traceFunctionListAppend(traceFunctionNode* node) {
	if (traceFunctionList != NULL) {
	  traceFunctionList->prev = node;
	  node->next = traceFunctionList;
	}
	traceFunctionList = node;
}

/* Get the Function name spezified by the given address. If there isn't a name, return NULL */
char *traceGetFuncName(Word address) {
	traceFunctionNode* nodebuffer;
	nodebuffer = traceFunctionList;
	while (nodebuffer != NULL && nodebuffer->address != address) {
		nodebuffer = nodebuffer->next;
	}
	if (nodebuffer == NULL) {
		return NULL;
	}
	return nodebuffer->name;
}

/* Get the address of the given symbol; 0 if not found */
Word traceGetAddress(char *name) {
	traceFunctionNode* nodebuffer;
	nodebuffer = traceFunctionList;
	while (nodebuffer != NULL) {
		if(strcmp(nodebuffer->name,name) == 0)
			return nodebuffer->address;
		nodebuffer = nodebuffer->next;
	}
	return 0;
}

/*
 * Trace the Parse Map and Store the Functions into the traceFunctionNode
 */
unsigned int traceParseMap(char *mapName) {
	  FILE *mapfile;
	  enum { NAME = 0, SECTION = 1, ADDRESS = 2, END = 3, FAIL = 4 } reading;
	  char buffer[MAX_IDENT_LEN];
	  int buff_i;
	  unsigned int secStart = 0;
	  traceFunctionNode template;
	  traceFunctionNode *cpyNode;
	  char currSection;
	  char t;
	  Bool inSegments;
	  Bool wordready;
	  Bool lineready;
	  traceFunctionNode *start;

	  /* open mapfile for reading*/
	  mapfile = fopen(mapName, "r");
	  if (mapfile == NULL) {
	    printf("Error: cannot open map-file '%s'. Tracing will be disabled.\n", mapName);
	    return;
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
	  while (1) {
		  /* if wordready, save word and reset for next one */
		  if (wordready) {
			  switch (reading) {
			  case NAME :
				  template.name = (char*)malloc(sizeof(char)*buff_i);
				  strncpy(template.name,buffer,MAX_IDENT_LEN);
				  reading = SECTION;
				  break;
			  case SECTION :
				  currSection = buffer[0];
				  reading = ADDRESS;
				  break;
			  case ADDRESS :
				  sscanf(buffer,"0x%08X",&template.address);
				  reading = END;
				  break;
			  default:
				  reading = FAIL;
			  }
			  buff_i = 0;
			  wordready = false;
			  continue;
		  }
		  /* if lineready, try to generate a struct */
		  if (lineready) {
			  /* didnt reached the expected end? -> invalid file/break */
			  if (reading != END) {
				  break;
			  }
			  /* check if the entry is out of the code section = function */
			  if (currSection == 'C') {
				  cpyNode = (traceFunctionNode*)malloc(sizeof(traceFunctionNode));
				  memcpy(cpyNode,&template,sizeof(traceFunctionNode));
				  traceFunctionListAppend(cpyNode);
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
		  if (t == EOF) {
			  break;
		  }
		  /* newline = wordready,lineready, normaly struct ready. */
		  if (t == '\n') {
			  buffer[buff_i] = '\0';
			  wordready = true;
			  lineready = true;
			  t = getc(mapfile);
			  /* two newlines = start of segments, so break here */
			  if(t == '\n') {
				  inSegments = true;
				  break;
			  }
			  continue;
		  }
		  /* ignore spaces but if they occur the first time: wordready */
		  if (isspace(t)) {
			  buffer[buff_i] = '\0';
			  wordready = true;
			  while (isspace(t = getc(mapfile)));
			  continue;
		  }
		  buffer[buff_i++] = t;
		  /* get next char */
		  t = getc(mapfile);
	  }

	  /* now look for "CODE start 0x...." */
	  if(inSegments) {
		  /* read "CODE" */
		  fscanf(mapfile,"CODE%*[\t ]start 0x%08X",&secStart);

		  /* change addresses */
		  for(cpyNode = traceFunctionList; cpyNode != start; cpyNode = cpyNode->next) {
			  cpyNode->address += secStart;
		  }
	  }
	  return secStart;
}
