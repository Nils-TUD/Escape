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

#pragma once

#include <esc/common.h>
#include "../exec/env.h"
#include "../lang.h"

#define AST_ASSIGN_STMT			0
#define AST_BINARY_OP_EXPR		1
#define AST_CMP_OP_EXPR			2
#define AST_CONST_STR_EXPR		3
#define AST_DYN_STR_EXPR		4
#define AST_IF_STMT				5
#define AST_STMT_LIST			6
#define AST_UNARY_OP_EXPR		7
#define AST_VAR_EXPR			8
#define AST_INT_EXPR			9
#define AST_COMMAND				10
#define AST_CMDEXPR_LIST		11
#define AST_SUB_CMD				12
#define AST_REDIR_FD			13
#define AST_REDIR_FILE			14
#define AST_FOR_STMT			15
#define AST_EXPR_STMT			16
#define AST_DSTR_EXPR			17
#define AST_WHILE_STMT			18
#define AST_FUNC_STMT			19
#define AST_EXPR_LIST			20
#define AST_PROPERTY			21

typedef uchar tASTType;

typedef struct sASTNode sASTNode;
struct sASTNode {
	tASTType type;
	void *data;
};

/**
 * Prints the tree
 *
 * @param n the node to start with
 * @param layer the layer to start with (indention)
 */
void ast_printTree(sASTNode *n,uint layer);

/**
 * Executes the given node(-tree)
 *
 * @param e the environment
 * @param n the node
 * @return the value
 */
sValue *ast_execute(sEnv *e,sASTNode *n);

/**
 * Destroys the node recursively
 *
 * @param n the node
 */
void ast_destroy(sASTNode *n);
