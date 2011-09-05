/**
 * $Id: help.c 152 2011-04-05 18:49:38Z nasmussen $
 */

#include "common.h"
#include "mmix/io.h"
#include "cli/cmds.h"
#include "cli/cmd/help.h"

void cli_cmd_help(size_t argc,const sASTNode **argv) {
	UNUSED(argc);
	UNUSED(argv);
	for(size_t i = 0; ; i++) {
		const sCommand *cmd = cmds_get(i);
		if(cmd == NULL)
			break;
		for(size_t j = 0; ; j++) {
			if(cmd->variants[j].synopsis == NULL)
				break;
			mprintf("  %-5s %-20s %s\n",cmd->name,cmd->variants[j].synopsis,cmd->variants[j].desc);
		}
	}
}
