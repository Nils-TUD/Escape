/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#pragma once

#include <esc/common.h>
#include <esc/sllist.h>
#include "node.h"
#include "../exec/env.h"

typedef struct {
	sSLList *list;
} sCmdExprList;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Creates a command-expression-list-node
 *
 * @return the created node
 */
sASTNode *ast_createCmdExprList(void);

/**
 * Executes the given node(-tree)
 *
 * @param e the environment
 * @param n the node
 * @return the value
 */
sValue *ast_execCmdExprList(sEnv *e,sCmdExprList *n);

/**
 * Adds the given expression to the list
 *
 * @param l the list
 * @param s the expression
 */
sASTNode *ast_addCmdExpr(sASTNode *l,sASTNode *s);

/**
 * Prints this command-expression-list
 *
 * @param s the list
 * @param layer the layer
 */
void ast_printCmdExprList(sCmdExprList *s,uint layer);

/**
 * Destroys the given command-expression-list (should be called from ast_destroy() only!)
 *
 * @param n the list
 */
void ast_destroyCmdExprList(sCmdExprList *n);

#if defined(__cplusplus)
}
#endif
