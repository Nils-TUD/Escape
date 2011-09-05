/**
 * $Id: effects.h 150 2011-04-05 17:24:00Z nasmussen $
 */

#ifndef TREFFECT_H_
#define TREFFECT_H_

#include "common.h"
#include "cli/cmds.h"
#include "core/decoder.h"

void cli_cmd_effectsInit(void);
void cli_cmd_effects(size_t argc,const sASTNode **argv);

#endif /* TREFFECT_H_ */
