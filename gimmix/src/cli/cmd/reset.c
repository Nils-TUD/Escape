/**
 * $Id: reset.c 228 2011-06-11 20:02:45Z nasmussen $
 */

#include "common.h"
#include "cli/cmds.h"
#include "cli/cmd/reset.h"
#include "sim.h"
#include "mmix/io.h"

void cli_cmd_reset(size_t argc,const sASTNode **argv) {
	UNUSED(argv);
	if(argc != 0)
		cmds_throwEx(NULL);

	mprintf("Resetting...");
	sim_reset();
	cmds_reset();
	mprintf("\n");
}
