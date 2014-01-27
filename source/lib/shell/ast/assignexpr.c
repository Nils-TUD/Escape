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
#include "assignexpr.h"
#include "varexpr.h"
#include "node.h"
#include "../mem.h"

sASTNode *ast_createAssignExpr(sASTNode *var,sASTNode *expr,bool hasIndex,sASTNode *index) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sAssignExpr *stmt = node->data = emalloc(sizeof(sAssignExpr));
	stmt->expr = expr;
	stmt->var = var;
	stmt->hasIndex = hasIndex;
	stmt->index = index;
	node->type = AST_ASSIGN_STMT;
	return node;
}

sValue *ast_execAssignExpr(sEnv *e,sAssignExpr *n) {
	sValue *v = ast_execute(e,n->expr);
	/* TODO maybe we should remove the var-expression? */
	const char *name = ((sVarExpr*)n->var->data)->name;
	if(n->hasIndex) {
		sValue *array = env_get(e,name);
		if(n->index) {
			sValue *index = ast_execute(e,n->index);
			val_setIndex(array,index,v);
		}
		else
			val_append(array,v);
		return val_clone(array);
	}
	else {
		/* don't free v since its kept in the env */
		v = env_set(e,name,v);
		/* we have to clone it because the user of this method may destroy it if its no longer needed */
		return val_clone(v);
	}
}

void ast_printAssignExpr(sAssignExpr *s,uint layer) {
	ast_printTree(s->var,layer);
	if(s->hasIndex) {
		printf("[");
		if(s->index)
			ast_printTree(s->index,layer);
		printf("]");
	}
	printf(" := ");
	ast_printTree(s->expr,layer);
}

void ast_destroyAssignExpr(sAssignExpr *n) {
	ast_destroy(n->index);
	ast_destroy(n->expr);
	ast_destroy(n->var);
}
