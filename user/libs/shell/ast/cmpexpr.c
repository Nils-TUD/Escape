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
#include "cmpexpr.h"
#include "node.h"
#include "../mem.h"

sASTNode *ast_createCmpExpr(sASTNode *operand1,u8 operation,sASTNode *operand2) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sCmpExpr *expr = node->data = emalloc(sizeof(sCmpExpr));
	expr->operand1 = operand1;
	expr->operation = operation;
	expr->operand2 = operand2;
	node->type = AST_CMP_OP_EXPR;
	return node;
}

sValue *ast_execCmpExpr(sEnv *e,sCmpExpr *n) {
	sValue *v1 = ast_execute(e,n->operand1);
	sValue *v2 = ast_execute(e,n->operand2);
	sValue *res = val_cmp(v1,v2,n->operation);
	val_destroy(v1);
	val_destroy(v2);
	return res;
}

void ast_printCmpExpr(sCmpExpr *s,u32 layer) {
	ast_printTree(s->operand1,layer);
	switch(s->operation) {
		case CMP_OP_EQ:
			printf(" == ");
			break;
		case CMP_OP_NEQ:
			printf(" == ");
			break;
		case CMP_OP_LT:
			printf(" < ");
			break;
		case CMP_OP_LEQ:
			printf(" <= ");
			break;
		case CMP_OP_GT:
			printf(" > ");
			break;
		case CMP_OP_GEQ:
			printf(" >= ");
			break;
	}
	ast_printTree(s->operand2,layer);
}

void ast_destroyCmpExpr(sCmpExpr *n) {
	ast_destroy(n->operand1);
	ast_destroy(n->operand2);
}
