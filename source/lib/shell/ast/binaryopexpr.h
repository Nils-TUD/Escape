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
	char operation;
	sASTNode *operand1;
	sASTNode *operand2;
} sBinaryOpExpr;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Creates an binary-op-node with: <operand1> <operation> <operand2>
 *
 * @param operand1 the first operand
 * @param operation the operation
 * @param operand2 the second operand
 * @return the created node
 */
sASTNode *ast_createBinOpExpr(sASTNode *operand1,char operation,sASTNode *operand2);

/**
 * Executes the given node(-tree)
 *
 * @param e the environment
 * @param n the node
 * @return the value
 */
sValue *ast_execBinOpExpr(sEnv *e,sBinaryOpExpr *n);

/**
 * Prints this expression
 *
 * @param s the expression
 * @param layer the layer
 */
void ast_printBinOpExpr(sBinaryOpExpr *s,uint layer);

/**
 * Destroys the given binary-op-expression (should be called from ast_destroy() only!)
 *
 * @param n the expression
 */
void ast_destroyBinOpExpr(sBinaryOpExpr *n);

#if defined(__cplusplus)
}
#endif
