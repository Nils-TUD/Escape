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
#include "node.h"
#include "../exec/env.h"

#define UN_OP_NEG		0
#define UN_OP_PREINC	1
#define UN_OP_POSTINC	2
#define UN_OP_PREDEC	3
#define UN_OP_POSTDEC	4

typedef struct {
	uchar operation;
	sASTNode *operand1;
} sUnaryOpExpr;

/**
 * Creates a unary-op-expression-node
 *
 * @param expr the expression
 * @param op the operation
 * @return the created node
 */
sASTNode *ast_createUnaryOpExpr(sASTNode *expr,uchar op);

/**
 * Executes the given node(-tree)
 *
 * @param e the environment
 * @param n the node
 * @return the value
 */
sValue *ast_execUnaryOpExpr(sEnv *e,sUnaryOpExpr *n);

/**
 * Prints this expression
 *
 * @param s the list
 * @param layer the layer
 */
void ast_printUnaryOpExpr(sUnaryOpExpr *s,uint layer);

/**
 * Destroys the given expression (should be called from ast_destroy() only!)
 *
 * @param n the expression
 */
void ast_destroyUnaryOpExpr(sUnaryOpExpr *n);
