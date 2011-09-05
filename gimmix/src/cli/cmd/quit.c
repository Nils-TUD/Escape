/**
 * $Id: quit.c 152 2011-04-05 18:49:38Z nasmussen $
 */

#include "common.h"
#include "cli/cmds.h"
#include "cli/cmd/quit.h"
#include "cli/console.h"

void cli_cmd_quit(size_t argc,const sASTNode **argv) {
	UNUSED(argc);
	UNUSED(argv);
	cons_stop();
}
