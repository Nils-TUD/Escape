/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SUBCMD_H_
#define SUBCMD_H_

#include <esc/common.h>
#include "node.h"
#include "../exec/env.h"

typedef struct {
	sASTNode *exprList;
	sASTNode *redirFd;
	sASTNode *redirIn;
	sASTNode *redirOut;
} sSubCmd;

/**
 * Creates an if-statement-node with the condition, then- and else-list
 *
 * @param exprList the list of expressions
 * @param redirFd the fd-redirections
 * @param redirIn the input-file
 * @param redirOut the output-file
 * @return the created node
 */
sASTNode *ast_createSubCmd(sASTNode *exprList,sASTNode *redirFd,sASTNode *redirIn,sASTNode *redirOut);

/**
 * Executes the given node(-tree)
 *
 * @param e the environment
 * @param n the node
 * @return the value
 */
sValue *ast_execSubCmd(sEnv *e,sSubCmd *n);

/**
 * Prints this command
 *
 * @param s the command
 * @param layer the layer
 */
void ast_printSubCmd(sSubCmd *s,u32 layer);

/**
 * Destroys the given command (should be called from ast_destroy() only!)
 *
 * @param n the command
 */
void ast_destroySubCmd(sSubCmd *n);

#endif /* SUBCMD_H_ */
