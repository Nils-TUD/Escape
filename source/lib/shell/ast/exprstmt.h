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

#include <sys/common.h>

#include "../exec/env.h"
#include "node.h"

typedef struct {
	sASTNode *expr;
} sExprStmt;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Creates an expression-statement-node
 *
 * @param expr the expression
 * @return the created node
 */
sASTNode *ast_createExprStmt(sASTNode *expr);

/**
 * Executes the given node(-tree)
 *
 * @param e the environment
 * @param n the node
 * @return the value
 */
sValue *ast_execExprStmt(sEnv *e,sExprStmt *n);

/**
 * Prints this statement
 *
 * @param s the statement
 * @param layer the layer
 */
void ast_printExprStmt(sExprStmt *s,uint layer);

/**
 * Destroys the given statement (should be called from ast_destroy() only!)
 *
 * @param n the statement
 */
void ast_destroyExprStmt(sExprStmt *n);

#if defined(__cplusplus)
}
#endif
