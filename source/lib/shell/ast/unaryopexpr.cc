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

#include "../mem.h"
#include "node.h"
#include "unaryopexpr.h"
#include "varexpr.h"

static sValue *ast_execUnaryPreOp(sEnv *e,sUnaryOpExpr *n,sValue *v,tIntType inc);
static sValue *ast_execUnaryPostOp(sEnv *e,sUnaryOpExpr *n,sValue *v,tIntType inc);

sASTNode *ast_createUnaryOpExpr(sASTNode *expr,uchar op) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sUnaryOpExpr *res = (sUnaryOpExpr*)emalloc(sizeof(sUnaryOpExpr));
	node->data = res;
	res->operand1 = expr;
	res->operation = op;
	node->type = AST_UNARY_OP_EXPR;
	return node;
}

sValue *ast_execUnaryOpExpr(sEnv *e,sUnaryOpExpr *n) {
	sValue *v = ast_execute(e,n->operand1);
	sValue *res = NULL;
	switch(n->operation) {
		case UN_OP_NEG:
			res = val_createInt(-val_getInt(v));
			break;
		case UN_OP_PREINC:
			res = ast_execUnaryPreOp(e,n,v,1);
			break;
		case UN_OP_POSTINC:
			res = ast_execUnaryPostOp(e,n,v,1);
			break;
		case UN_OP_PREDEC:
			res = ast_execUnaryPreOp(e,n,v,-1);
			break;
		case UN_OP_POSTDEC:
			res = ast_execUnaryPostOp(e,n,v,-1);
			break;
	}
	val_destroy(v);
	return res;
}

void ast_printUnaryOpExpr(sUnaryOpExpr *s,uint layer) {
	switch(s->operation) {
		case UN_OP_NEG:
			printf("-");
			break;
		case UN_OP_PREINC:
			printf("++");
			break;
		case UN_OP_PREDEC:
			printf("--");
			break;
	}
	ast_printTree(s->operand1,layer);
	switch(s->operation) {
		case UN_OP_POSTINC:
			printf("++");
			break;
		case UN_OP_POSTDEC:
			printf("--");
			break;
	}
}

void ast_destroyUnaryOpExpr(sUnaryOpExpr *l) {
	ast_destroy(l->operand1);
}

static sValue *ast_execUnaryPreOp(sEnv *e,sUnaryOpExpr *n,sValue *v,tIntType inc) {
	/* its always a variable */
	const char *name = ((sVarExpr*)n->operand1->data)->name;
	/* don't free it since its kept in the env */
	sValue *res = val_createInt(val_getInt(v) + inc);
	res = env_set(e,name,res);
	/* we have to clone it because the user of this method may destroy it if its no longer needed */
	return val_clone(res);
}

static sValue *ast_execUnaryPostOp(sEnv *e,sUnaryOpExpr *n,sValue *v,tIntType inc) {
	/* its always a variable */
	const char *name = ((sVarExpr*)n->operand1->data)->name;
	/* don't free it since its kept in the env */
	sValue *res = val_createInt(val_getInt(v) + inc);
	res = env_set(e,name,res);
	/* we have to clone it because the user of this method may destroy it if its no longer needed */
	return val_createInt(val_getInt(res) - inc);
}
