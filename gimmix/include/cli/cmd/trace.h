/**
 * $Id: trace.h 171 2011-04-14 13:18:30Z nasmussen $
 */

#ifndef TRACE_H_
#define TRACE_H_

#include "common.h"
#include "cli/cmds.h"

void cli_cmd_traceInit(void);
void cli_cmd_traceShutdown(void);
void cli_cmd_trace(size_t argc,const sASTNode **argv);
void cli_cmd_delTrace(size_t argc,const sASTNode **argv);

#endif /* TRACE_H_ */
