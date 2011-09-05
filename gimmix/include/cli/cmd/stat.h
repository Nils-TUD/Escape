/**
 * $Id: stat.h 228 2011-06-11 20:02:45Z nasmussen $
 */

#ifndef STAT_H_
#define STAT_H_

#include "common.h"
#include "cli/cmds.h"

void cli_cmd_statInit(void);
void cli_cmd_statReset(void);
void cli_cmd_stat(size_t argc,const sASTNode **argv);

#endif /* STAT_H_ */
