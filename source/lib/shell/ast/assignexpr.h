/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#ifndef ASSIGNSTMT_H_
#define ASSIGNSTMT_H_

#include <esc/common.h>
#include "node.h"
#include "../exec/env.h"

typedef struct {
	sASTNode *var;
	sASTNode *expr;
	bool hasIndex;
	sASTNode *index;
} sAssignExpr;

/**
 * Creates an assign-node with given variable and the expression to evaluate.
 *
 * @param var the variable
 * @param expr the expression
 * @param hasIndex whether the assignment has an index
 * @param index the index (NULL for '[]')
 * @return the created node
 */
sASTNode *ast_createAssignExpr(sASTNode *var,sASTNode *expr,bool hasIndex,sASTNode *index);

/**
 * Executes the given node(-tree)
 *
 * @param e the environment
 * @param n the node
 * @return the value
 */
sValue *ast_execAssignExpr(sEnv *e,sAssignExpr *n);

/**
 * Prints this expression
 *
 * @param s the expression
 * @param layer the layer
 */
void ast_printAssignExpr(sAssignExpr *s,uint layer);

/**
 * Destroys the given assign-expression (should be called from ast_destroy() only!)
 *
 * @param n the expression
 */
void ast_destroyAssignExpr(sAssignExpr *n);

#endif /* ASSIGNSTMT_H_ */
