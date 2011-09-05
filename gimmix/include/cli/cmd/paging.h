/**
 * $Id: paging.h 153 2011-04-06 16:46:17Z nasmussen $
 */

#ifndef PAGING_H_
#define PAGING_H_

#include "common.h"
#include "cli/cmds.h"

void cli_cmd_pagingInit(void);
void cli_cmd_v2p(size_t argc,const sASTNode **argv);
void cli_cmd_itc(size_t argc,const sASTNode **argv);
void cli_cmd_dtc(size_t argc,const sASTNode **argv);

#endif /* PAGING_H_ */
