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
#include "binaryopexpr.h"
#include "node.h"
#include "../mem.h"

sASTNode *ast_createBinOpExpr(sASTNode *operand1,char operation,sASTNode *operand2) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sBinaryOpExpr *expr = node->data = emalloc(sizeof(sBinaryOpExpr));
	expr->operand1 = operand1;
	expr->operation = operation;
	expr->operand2 = operand2;
	node->type = AST_BINARY_OP_EXPR;
	return node;
}

void ast_printBinOpExpr(sBinaryOpExpr *s,u32 layer) {
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
