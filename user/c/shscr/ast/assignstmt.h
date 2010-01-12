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

#ifndef ASSIGNSTMT_H_
#define ASSIGNSTMT_H_

#include <esc/common.h>
#include "node.h"

typedef struct {
	sASTNode *var;
	sASTNode *expr;
} sAssignStmt;

/**
 * Creates an assign-node with given variable and the expression to evaluate.
 *
 * @param var the variable
 * @param expr the expression
 * @return the created node
 */
sASTNode *ast_createAssignStmt(sASTNode *var,sASTNode *expr);

/**
 * Prints this statement
 *
 * @param s the statement
 * @param layer the layer
 */
void ast_printAssignStmt(sAssignStmt *s,u32 layer);

/**
 * Destroys the given assign-statement (should be called from ast_destroy() only!)
 *
 * @param n the statement
 */
void ast_destroyAssignStmt(sAssignStmt *n);

#endif /* ASSIGNSTMT_H_ */
