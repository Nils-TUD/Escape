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

#include <sys/common.h>
#include <stdio.h>
#include "conststrexpr.h"
#include "node.h"
#include "../mem.h"

sASTNode *ast_createConstStrExpr(char *s,bool hasQuotes) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sConstStrExpr *expr = (sConstStrExpr*)emalloc(sizeof(sConstStrExpr));
	node->data = expr;
	/* no clone necessary here because we've already cloned it in the scanner */
	expr->str = s;
	expr->hasQuotes = hasQuotes;
	node->type = AST_CONST_STR_EXPR;
	return node;
}

sValue *ast_execConstStrExpr(A_UNUSED sEnv *e,sConstStrExpr *n) {
	return val_createStr(n->str);
}

void ast_printConstStrExpr(sConstStrExpr *s,A_UNUSED uint layer) {
	printf("'%s'",s->str);
}

void ast_destroyConstStrExpr(sConstStrExpr *n) {
	efree(n->str);
}
