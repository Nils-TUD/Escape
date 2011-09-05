/**
 * $Id: trace.c 217 2011-06-03 18:11:57Z nasmussen $
 */

#include <string.h>

#include "common.h"
#include "cli/cmds.h"
#include "cli/cmd/trace.h"
#include "cli/console.h"
#include "event.h"
#include "mmix/io.h"
#include "mmix/mem.h"

#define MAX_TRACE_CMDS	16
#define MAX_CMD_LEN		255

static void cpuPause(const sEvArgs *args);
static void printTraces(void);
static int freeSlot(void);

static char *traceCmds[MAX_TRACE_CMDS];

void cli_cmd_traceInit(void) {
	ev_register(EV_CPU_PAUSE,cpuPause);
}

void cli_cmd_trace(size_t argc,const sASTNode **argv) {
	if(argc == 0)
		printTraces();
	else {
		int slot = freeSlot();
		if(slot == -1)
			cmds_throwEx("Can't add another trace-command! Limit reached.\n");

		size_t size = MAX_CMD_LEN;
		char *cmd = (char*)mem_alloc(MAX_CMD_LEN);
		char *curCmd = cmd;
		for(size_t i = 0; i < argc; i++) {
			size_t len = ast_toString(argv[i],curCmd,size);
			size -= len + 1;
			if(size < 2 || i >= argc - 1)
				break;
			curCmd[len] = ' ';
			curCmd[len + 1] = '\0';
			curCmd += len + 1;
		}

		mprintf("Added command '%s' in slot %d\n",cmd,slot);
		traceCmds[slot] = cmd;
	}
}

void cli_cmd_delTrace(size_t argc,const sASTNode **argv) {
	if(argc != 1)
		cmds_throwEx(NULL);

	for(octa offset = 0; ; offset++) {
		bool finished = false;
		sCmdArg no = cmds_evalArg(argv[0],ARGVAL_INT,offset,&finished);
		if(finished)
			break;

		size_t i = no.d.integer;
		if(i >= MAX_TRACE_CMDS || traceCmds[i] == NULL)
			cmds_throwEx("Invalid trace-number!\n");
		mprintf("Removed command '%s' from slot %d\n",traceCmds[i],(int)i);
		mem_free(traceCmds[i]);
		traceCmds[i] = NULL;
	}
}

static void cpuPause(const sEvArgs *args) {
	UNUSED(args);
	for(size_t i = 0; i < MAX_TRACE_CMDS; i++) {
		if(traceCmds[i])
			cons_exec(traceCmds[i],false);
	}
}

static void printTraces(void) {
	for(size_t i = 0; i < MAX_TRACE_CMDS; i++) {
		if(traceCmds[i])
			mprintf("%02d: %s\n",(int)i,traceCmds[i]);
	}
}

static int freeSlot(void) {
	for(size_t i = 0; i < MAX_TRACE_CMDS; i++) {
		if(traceCmds[i] == NULL)
			return i;
	}
	return -1;
}
