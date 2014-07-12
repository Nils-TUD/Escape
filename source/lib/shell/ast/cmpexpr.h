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
#include "node.h"
#include "../exec/env.h"

#define CMP_OP_EQ	0
#define CMP_OP_NEQ	1
#define CMP_OP_LT	2
#define CMP_OP_GT	3
#define CMP_OP_LEQ	4
#define CMP_OP_GEQ	5

typedef struct {
	uint operation;
	sASTNode *operand1;
	sASTNode *operand2;
} sCmpExpr;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Creates an compare-node with: <operand1> <operation> <operand2>
 *
 * @param operand1 the first operand
 * @param operation the operation
 * @param operand2 the second operand
 * @return the created node
 */
sASTNode *ast_createCmpExpr(sASTNode *operand1,uint operation,sASTNode *operand2);

/**
 * Executes the given node(-tree)
 *
 * @param e the environment
 * @param n the node
 * @return the value
 */
sValue *ast_execCmpExpr(sEnv *e,sCmpExpr *n);

/**
 * Prints this expression
 *
 * @param s the expression
 * @param layer the layer
 */
void ast_printCmpExpr(sCmpExpr *s,uint layer);

/**
 * Destroys the given compare-expression (should be called from ast_destroy() only!)
 *
 * @param n the expression
 */
void ast_destroyCmpExpr(sCmpExpr *n);

#if defined(__cplusplus)
}
#endif
