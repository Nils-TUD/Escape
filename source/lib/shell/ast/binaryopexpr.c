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

#include <esc/common.h>
#include <stdio.h>
#include "binaryopexpr.h"
#include "node.h"
#include "../mem.h"

static tIntType pow(tIntType a,tIntType b);

sASTNode *ast_createBinOpExpr(sASTNode *operand1,char operation,sASTNode *operand2) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sBinaryOpExpr *expr = node->data = emalloc(sizeof(sBinaryOpExpr));
	expr->operand1 = operand1;
	expr->operation = operation;
	expr->operand2 = operand2;
	node->type = AST_BINARY_OP_EXPR;
	return node;
}

sValue *ast_execBinOpExpr(sEnv *e,sBinaryOpExpr *n) {
	sValue *v1 = ast_execute(e,n->operand1);
	sValue *v2 = ast_execute(e,n->operand2);
	sValue *res = NULL;
	switch(n->operation) {
		case '+':
			res = val_createInt(val_getInt(v1) + val_getInt(v2));
			break;
		case '-':
			res = val_createInt(val_getInt(v1) - val_getInt(v2));
			break;
		case '*':
			res = val_createInt(val_getInt(v1) * val_getInt(v2));
			break;
		case '/':
			/* TODO division by zero */
			res = val_createInt(val_getInt(v1) / val_getInt(v2));
			break;
		case '%':
			res = val_createInt(val_getInt(v1) % val_getInt(v2));
			break;
		case '^':
			res = val_createInt(pow(val_getInt(v1),val_getInt(v2)));
			break;
	}
	/* we don't need the values anymore, so delete them */
	val_destroy(v1);
	val_destroy(v2);
	return res;
}

void ast_printBinOpExpr(sBinaryOpExpr *s,uint layer) {
	printf("(");
	ast_printTree(s->operand1,layer);
	printf(" %c ",s->operation);
	ast_printTree(s->operand2,layer);
	printf(")");
}

void ast_destroyBinOpExpr(sBinaryOpExpr *n) {
	ast_destroy(n->operand1);
	ast_destroy(n->operand2);
}

static tIntType pow(tIntType a,tIntType b) {
	tIntType i,res = 1;
	for(i = 0; i < b; i++)
		res *= a;
	return res;
}
