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
#include "unaryopexpr.h"
#include "node.h"
#include "../mem.h"

sASTNode *ast_createUnaryOpExpr(sASTNode *expr,u8 op) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sUnaryOpExpr *res = node->data = emalloc(sizeof(sUnaryOpExpr));
	res->operand1 = expr;
	res->operation = op;
	node->type = AST_UNARY_OP_EXPR;
	return node;
}

void ast_printUnaryOpExpr(sUnaryOpExpr *s,u32 layer) {
	switch(s->operation) {
		case UN_OP_NEG:
			printf("-");
			break;
	}
	ast_printTree(s->operand1,layer);
}

void ast_destroyUnaryOpExpr(sUnaryOpExpr *l) {
	ast_destroy(l->operand1);
}
