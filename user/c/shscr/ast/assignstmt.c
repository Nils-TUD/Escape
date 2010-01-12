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

#include <esc/common.h>
#include <esc/fileio.h>
#include "assignstmt.h"
#include "node.h"
#include "../mem.h"

sASTNode *ast_createAssignStmt(sASTNode *var,sASTNode *expr) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sAssignStmt *stmt = node->data = emalloc(sizeof(sAssignStmt));
	stmt->expr = expr;
	stmt->var = var;
	node->type = AST_ASSIGN_STMT;
	return node;
}

void ast_printAssignStmt(sAssignStmt *s,u32 layer) {
	printf("%*s",layer,"");
	ast_printTree(s->var,layer);
	printf(" = ");
	ast_printTree(s->expr,layer);
	printf("\n");
}

void ast_destroyAssignStmt(sAssignStmt *n) {
	ast_destroy(n->expr);
	ast_destroy(n->var);
}
