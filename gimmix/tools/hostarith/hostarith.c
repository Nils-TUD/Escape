/**
 * $Id: hostarith.c 239 2011-08-30 18:28:06Z nasmussen $
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "common.h"
#include "mmix/io.h"
#include "mmix/error.h"
#include "mmix/mem.h"

#define MAX_LINE_LEN	255

int main(void) {
	static char line[MAX_LINE_LEN];
	mprintf("Commands:\n");
	mprintf("  - <op1> div|divu|mul|mul|mod|modu|sl|sr|sar <op2>\n");
	mprintf("  - q\n");
	mprintf("\n");
	while(1) {
		mprintf("> ");
		if(!fgets(line,sizeof(line),stdin) || strcmp(line,"q\n") == 0)
			break;

		// split line by whitespace into op1,op and op2
		char *tok = strtok(line," \r\t");
		octa op1 = mstrtoo(tok,NULL,0);
		if((tok = strtok(NULL," \r\t")) == NULL) {
			mprintf("Invalid input\n");
			continue;
		}
		char *op = mem_alloc(strlen(tok) + 1);
		strcpy(op,tok);
		if((tok = strtok(NULL," \r\t")) == NULL) {
			mprintf("Invalid input\n");
			mem_free(op);
			continue;
		}
		octa op2 = mstrtoo(tok,NULL,0);
		while(strtok(NULL," \r\t"));

		if(strcmp(op,"div") == 0) {
			socta res = (socta)op1 / (socta)op2;
			mprintf("%Od div %Od = %Od\n",op1,op2,res);
		}
		else if(strcmp(op,"divu") == 0) {
			octa res = op1 / op2;
			mprintf("%#Ox divu %#Ox = %#Ox\n",op1,op2,res);
		}
		else if(strcmp(op,"mul") == 0) {
			socta res = (socta)op1 * (socta)op2;
			mprintf("%Od mul %Od = %Od\n",op1,op2,res);
		}
		else if(strcmp(op,"mulu") == 0) {
			octa res = op1 * op2;
			mprintf("%#Ox mulu %#Ox = %#Ox\n",op1,op2,res);
		}
		else if(strcmp(op,"mod") == 0) {
			socta res = (socta)op1 % (socta)op2;
			mprintf("%Od mod %Od = %Od\n",op1,op2,res);
		}
		else if(strcmp(op,"modu") == 0) {
			octa res = op1 % op2;
			mprintf("%#Ox modu %#Ox = %#Ox\n",op1,op2,res);
		}
		else if(strcmp(op,"sl") == 0) {
			octa res = op1 << op2;
			mprintf("%#Ox sl %#Ox = %#Ox\n",op1,op2,res);
		}
		else if(strcmp(op,"sr") == 0) {
			octa res = op1 >> op2;
			mprintf("%#Ox sr %#Ox = %#Ox\n",op1,op2,res);
		}
		else if(strcmp(op,"sar") == 0) {
			octa res = (socta)op1 >> op2;
			mprintf("%#Ox sar %#Ox = %#Ox\n",op1,op2,res);
		}
		else
			mprintf("Unknown operation\n");
		mem_free(op);
	}

	return EXIT_SUCCESS;
}
