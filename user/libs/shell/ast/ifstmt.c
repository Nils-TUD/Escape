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
#include <stdio.h>
#include "ifstmt.h"
#include "node.h"
#include "../mem.h"

sASTNode *ast_createIfStmt(sASTNode *cond,sASTNode *thenList,sASTNode *elseList) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sIfStmt *expr = node->data = emalloc(sizeof(sIfStmt));
	expr->cond = cond;
	expr->thenList = thenList;
	expr->elseList = elseList;
	node->type = AST_IF_STMT;
	return node;
}

sValue *ast_execIfStmt(sEnv *e,sIfStmt *n) {
	sValue *cond = ast_execute(e,n->cond);
	sValue *res;
	if(val_isTrue(cond))
		res = ast_execute(e,n->thenList);
	else
		res = ast_execute(e,n->elseList);
	val_destroy(cond);
	val_destroy(res);
	return NULL;
}

void ast_printIfStmt(sIfStmt *s,u32 layer) {
	printf("%*sif ( ",layer,"");
	ast_printTree(s->cond,layer);
	printf(" )\n");
	ast_printTree(s->thenList,layer + 1);
	if(s->elseList) {
		printf("%*selse\n",layer,"");
		ast_printTree(s->elseList,layer + 1);
	}
	printf("%*sfi\n",layer,"");
}

void ast_destroyIfStmt(sIfStmt *n) {
	ast_destroy(n->cond);
	ast_destroy(n->thenList);
	ast_destroy(n->elseList);
}
