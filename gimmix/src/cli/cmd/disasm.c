/**
 * $Id: disasm.c 217 2011-06-03 18:11:57Z nasmussen $
 */

#include "common.h"
#include "cli/cmd/disasm.h"
#include "cli/lang/specreg.h"
#include "cli/lang/idesc.h"
#include "core/cpu.h"
#include "core/mmu.h"
#include "core/decoder.h"
#include "exception.h"
#include "mmix/io.h"
#include "mmix/error.h"

#define DEFAULT_INSTR_COUNT		16

static void doDisasm(octa loc);

void cli_cmd_disasm(size_t argc,const sASTNode **argv) {
	if(argc > 1)
		cmds_throwEx(NULL);

	if(argc == 0) {
		for(int i = 0; i < DEFAULT_INSTR_COUNT; i++)
			doDisasm(cpu_getPC() + i * sizeof(tetra));
	}
	else {
		bool finished = false;
		for(octa offset = 0; ; offset += sizeof(tetra)) {
			sCmdArg res = cmds_evalArg(argv[0],ARGVAL_INT,offset,&finished);
			if(finished)
				break;
			doDisasm(res.d.integer & ~(octa)(sizeof(tetra) - 1));
		}
	}
}

static void doDisasm(octa loc) {
	tetra instr = mmu_readTetra(loc,0);
	mprintf("%016OX: %s\n",loc,idesc_disasm(loc,instr,true));
}
