/**
 * $Id: cache.h 152 2011-04-05 18:49:38Z nasmussen $
 */

#ifndef CLI_CACHE_H_
#define CLI_CACHE_H_

#include "common.h"
#include "cli/cmds.h"

void cli_cmd_ic(size_t argc,const sASTNode **argv);
void cli_cmd_dc(size_t argc,const sASTNode **argv);

#endif /* CLI_CACHE_H_ */
