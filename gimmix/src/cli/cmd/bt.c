/**
 * $Id$
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "common.h"
#include "core/cpu.h"
#include "core/mmu.h"
#include "core/register.h"
#include "cli/console.h"
#include "cli/cmds.h"
#include "cli/cmd/bt.h"
#include "cli/lang/bt.h"
#include "cli/lang/btsimple.h"
#include "cli/lang/bttree.h"
#include "cli/lang/symmap.h"
#include "getline/getline.h"
#include "mmix/io.h"
#include "event.h"
#include "sim.h"
#include "exception.h"

typedef struct {
	octa id;
	octa instrCount;
} sTrace;

#define TRACES_PER_LINE		5

static int ex = EX_NONE;
static octa exBits;
static octa oldPC = 0;

/* our traces */
static size_t traceCount = 1; /* the first trace is "all" */
static sTrace traces[MAX_TRACES];
static octa lastIndex = -1;
static octa lastId = -1;

static void afterExec(const sEvArgs *args);
static void exception(const sEvArgs *args);
static octa getId(void);
static tTraceId readTraceId(void);
static tTraceId getTraceId(void);

void cli_cmd_btInit(void) {
	if(cfg_useBacktracing()) {
		const char **mapNames = cfg_getMaps();
		if(mapNames[0] != NULL) {
			for(size_t i = 0; i < MAX_MAPS; i++) {
				if(mapNames[i] == NULL)
					break;
				symmap_parse(mapNames[i]);
			}

			btsimple_init();
			bttree_init();
			ev_register(EV_AFTER_EXEC,afterExec);
			ev_register(EV_EXCEPTION,exception);
		}
	}
	cli_cmd_btReset();
}

void cli_cmd_btReset(void) {
	if(cfg_useBacktracing()) {
		traceCount = 1;
		for(size_t i = 0; i < MAX_TRACES; i++) {
			traces[i].id = 0x800000000000000;
			traces[i].instrCount = 0;
		}
		btsimple_cleanAll();
		bttree_cleanAll();
		btsimple_init();
		bttree_init();
	}
}

void cli_cmd_delBt(size_t argc,const sASTNode **argv) {
	tTraceId id;
	if(!cfg_useBacktracing())
		cmds_throwEx("Sorry, backtracing is disabled.\n");
	if(argc > 1)
		cmds_throwEx(NULL);

	if(argc < 1)
		id = readTraceId();
	else {
		bool finished = false;
		sCmdArg arg = cmds_evalArg(argv[0],ARGVAL_INT,0,&finished);
		id = arg.d.integer;
	}

	if(id >= traceCount)
		cmds_throwEx("Illegal backtrace!\n");
	else {
		btsimple_clean(id);
		bttree_clean(id);
	}
}

void cli_cmd_bt(size_t argc,const sASTNode **argv) {
	tTraceId id;
	if(!cfg_useBacktracing())
		cmds_throwEx("Sorry, backtracing is disabled.\n");
	if(argc > 1)
		cmds_throwEx(NULL);

	if(argc < 1)
		id = readTraceId();
	else {
		bool finished = false;
		sCmdArg arg = cmds_evalArg(argv[0],ARGVAL_INT,0,&finished);
		id = arg.d.integer;
	}

	if(id >= traceCount)
		cmds_throwEx("Illegal backtrace!\n");
	else
		btsimple_print(id);
}

void cli_cmd_btTree(size_t argc,const sASTNode **argv) {
	tTraceId id;
	if(!cfg_useBacktracing())
		cmds_throwEx("Sorry, backtracing is disabled.\n");
	if(argc < 1)
		cmds_throwEx(NULL);

	if(argc < 2)
		id = readTraceId();
	else {
		bool finished = false;
		sCmdArg arg = cmds_evalArg(argv[1],ARGVAL_INT,0,&finished);
		id = arg.d.integer;
	}

	if(id >= traceCount)
		cmds_throwEx("Illegal trace!\n");
	else {
		bool finished = false;
		sCmdArg arg = cmds_evalArg(argv[0],ARGVAL_STR,0,&finished);
		bttree_print(id,arg.d.str);
	}
}

static void afterExec(const sEvArgs *args) {
	UNUSED(args);
	tTraceId id = getTraceId();
	tetra raw = cpu_getCurInstrRaw();
	if(OPCODE(raw) == PUSHGO || OPCODE(raw) == PUSHGOI || OPCODE(raw) == PUSHJ ||
			OPCODE(raw) == PUSHJB) {
		octa callargs[MAX_ARGS];
		size_t argCount = MIN(MAX_ARGS,reg_getSpecial(rL));
		for(size_t i = 0; i < argCount; i++)
			callargs[i] = reg_get(i);

		octa newPC = cpu_getPC() + sizeof(tetra);
		octa threadId = getId();
		if(id != 0) {
			btsimple_add(id,true,traces[id].instrCount,oldPC,newPC,threadId,0,0,argCount,callargs);
			bttree_add(id,true,traces[id].instrCount,oldPC,newPC,threadId,0,0,argCount,callargs);
		}
		btsimple_add(0,true,traces[0].instrCount,oldPC,newPC,threadId,0,0,argCount,callargs);
		bttree_add(0,true,traces[0].instrCount,oldPC,newPC,threadId,0,0,argCount,callargs);
	}
	else if(OPCODE(raw) == POP) {
		octa cpc = cpu_getPC() + sizeof(tetra);
		octa threadId = getId();
		if(id != 0) {
			btsimple_remove(id,true,traces[id].instrCount,cpc,threadId);
			bttree_remove(id,true,traces[id].instrCount,cpc,threadId);
		}
		btsimple_remove(0,true,traces[0].instrCount,cpc,threadId);
		bttree_remove(0,true,traces[0].instrCount,cpc,threadId);
	}
	else if(OPCODE(raw) == RESUME) {
		octa cpc = cpu_getPC() + sizeof(tetra);
		octa threadId = getId();
		if(id != 0) {
			btsimple_remove(id,false,traces[id].instrCount,cpc,threadId);
			bttree_remove(id,false,traces[id].instrCount,cpc,threadId);
		}
		btsimple_remove(0,false,traces[0].instrCount,cpc,threadId);
		bttree_remove(0,false,traces[0].instrCount,cpc,threadId);
	}
	if(ex != EX_NONE) {
		octa cpc = cpu_getPC();
		octa threadId = getId();
		if(id != 0) {
			btsimple_add(id,false,traces[id].instrCount,oldPC,cpc,threadId,ex,exBits,0,NULL);
			bttree_add(id,false,traces[id].instrCount,oldPC,cpc,threadId,ex,exBits,0,NULL);
		}
		btsimple_add(0,false,traces[0].instrCount,oldPC,cpc,threadId,ex,exBits,0,NULL);
		bttree_add(0,false,traces[0].instrCount,oldPC,cpc,threadId,ex,exBits,0,NULL);
		ex = EX_NONE;
	}

	traces[id].instrCount++;
	traces[0].instrCount = reg_getSpecial(rC);
	oldPC = cpu_getPC() + sizeof(tetra);
}

static void exception(const sEvArgs *args) {
	UNUSED(args);
	ex = ex_getEx();
	exBits = ex_getBits();
}

static octa getId(void) {
	return reg_getSpecial(rF);
}

static tTraceId readTraceId(void) {
	octa addrSpace = getId();
	mprintf("Please choose one of the following traces:\n");
	for(size_t i = 0; i < traceCount; i++) {
		if(i == 0)
			mprintf("  %2d:    all   ",i);
		else {
			if(traces[i].id == addrSpace)
				mprintf("  %2d: *%07OX*",i,traces[i].id);
			else
				mprintf("  %2d:  %07OX ",i,traces[i].id);
		}
		if(i < traceCount - 1 && i % TRACES_PER_LINE == (TRACES_PER_LINE - 1))
			mprintf("\n");
	}
	fflush(stdout);
	char *line = gl_getline((char*)"\nYour choice: ");
	unsigned int i;
	sscanf(line,"%u",&i);
	return i < traceCount ? i : MAX_TRACES;
}

static tTraceId getTraceId(void) {
	octa addrSpace = getId();
	if(addrSpace == lastId)
		return lastIndex;
	for(size_t i = 0; i < traceCount; i++) {
		if(addrSpace == traces[i].id) {
			lastId = traces[i].id;
			lastIndex = i;
			return i;
		}
	}

	if(traceCount == MAX_TRACES)
		cmds_throwEx("Max. number of traces reached!\n");

	traces[traceCount].id = addrSpace;
	return traceCount++;
}
