/**
 * $Id$
 */

#ifndef CMD_BT_H_
#define CMD_BT_H_

#include "common.h"

/*
 * Inits the trace-command
 */
void cli_cmd_btInit(void);

/**
 * Resets the trace-command
 */
void cli_cmd_btReset(void);

/**
 * Shuts the trace-command down
 */
void cli_cmd_btShutdown(void);

/**
 * Deletes a stack-trace
 */
void cli_cmd_delBt(size_t argc,const sASTNode **argv);

/*
 * Print a stack-trace
 */
void cli_cmd_bt(size_t argc,const sASTNode **argv);

/**
 * Print a complete trace to file
 */
void cli_cmd_btTree(size_t argc,const sASTNode **argv);

#endif /*CMD_BT_H_*/
