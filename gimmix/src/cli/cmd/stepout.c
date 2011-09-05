/**
 * $Id: stepout.c 202 2011-05-10 10:49:04Z nasmussen $
 */

#include "common.h"
#include "cli/cmds.h"
#include "cli/cmd/stepout.h"
#include "core/cpu.h"
#include "event.h"

static void afterExec(const sEvArgs *args);

static int level;

void cli_cmd_stepout(size_t argc,const sASTNode **argv) {
	if(argc != 0 && argc != 1)
		cmds_throwEx(NULL);

	octa count = 1;
	if(argc == 1) {
		sCmdArg first = cmds_evalArg(argv[0],ARGVAL_INT,0,NULL);
		count = first.d.integer;
	}

	if(cpu_isHalted())
		cmds_throwEx("CPU is halted. Set the PC or reset the machine!\n");
	ev_register(EV_AFTER_EXEC,afterExec);
	while(count-- > 0) {
		level = 1;
		cpu_run();
	}
	ev_unregister(EV_AFTER_EXEC,afterExec);
}

static void afterExec(const sEvArgs *args) {
	UNUSED(args);
	tetra raw = cpu_getCurInstrRaw();
	if(OPCODE(raw) == PUSHGO || OPCODE(raw) == PUSHGOI ||
		OPCODE(raw) == PUSHJ || OPCODE(raw) == PUSHJB)
		level++;
	else if(OPCODE(raw) == POP) {
		if(--level == 0)
			cpu_pause();
	}
}
