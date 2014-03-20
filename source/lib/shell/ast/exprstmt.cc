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

#include <esc/common.h>
#include <stdio.h>
#include "exprstmt.h"
#include "node.h"
#include "../mem.h"

sASTNode *ast_createExprStmt(sASTNode *expr) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sExprStmt *stmt = (sExprStmt*)emalloc(sizeof(sExprStmt));
	node->data = stmt;
	stmt->expr = expr;
	node->type = AST_EXPR_STMT;
	return node;
}

sValue *ast_execExprStmt(sEnv *e,sExprStmt *n) {
	return ast_execute(e,n->expr);
}

void ast_printExprStmt(sExprStmt *s,uint layer) {
	printf("%*s",layer,"");
	ast_printTree(s->expr,layer);
	printf(";\n");
}

void ast_destroyExprStmt(sExprStmt *n) {
	ast_destroy(n->expr);
}
