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
#include "whilestmt.h"
#include "node.h"
#include "../mem.h"

sASTNode *ast_createWhileStmt(sASTNode *condExpr,sASTNode *stmtList) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sWhileStmt *expr = node->data = emalloc(sizeof(sWhileStmt));
	expr->condExpr = condExpr;
	expr->stmtList = stmtList;
	node->type = AST_WHILE_STMT;
	return node;
}

sValue *ast_execWhileStmt(sEnv *e,sWhileStmt *n) {
	sValue *cond = ast_execute(e,n->condExpr);
	while(val_isTrue(cond)) {
		/* execute body */
		val_destroy(ast_execute(e,n->stmtList));
		/* evaluate condition */
		val_destroy(cond);
		cond = ast_execute(e,n->condExpr);
	}
	val_destroy(cond);
	return NULL;
}

void ast_printWhileStmt(sWhileStmt *s,u32 layer) {
	printf("%*swhile ( ",layer,"");
	ast_printTree(s->condExpr,layer);
	printf(" ) do\n");
	ast_printTree(s->stmtList,layer + 1);
	printf("%*sdone\n",layer,"");
}

void ast_destroyWhileStmt(sWhileStmt *n) {
	ast_destroy(n->condExpr);
	ast_destroy(n->stmtList);
}
