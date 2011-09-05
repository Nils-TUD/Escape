/**
 * $Id: break.h 171 2011-04-14 13:18:30Z nasmussen $
 */

#ifndef BREAK_H_
#define BREAK_H_

#include "common.h"
#include "cli/cmds.h"

void cli_cmd_breakInit(void);
void cli_cmd_break(size_t argc,const sASTNode **argv);
void cli_cmd_delBreak(size_t argc,const sASTNode **argv);

#endif /* BREAK_H_ */
