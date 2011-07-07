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
#include "varexpr.h"
#include "node.h"
#include "../mem.h"

sASTNode *ast_createVarExpr(char *s,sASTNode *index) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sVarExpr *expr = node->data = emalloc(sizeof(sVarExpr));
	/* no clone necessary here because we've already cloned it in the scanner */
	expr->name = s;
	expr->index = index;
	node->type = AST_VAR_EXPR;
	return node;
}

sValue *ast_execVarExpr(sEnv *e,sVarExpr *n) {
	sValue *v = env_get(e,n->name);
	/* we have to clone it because the user of this method may destroy it if its no longer needed */
	if(v) {
		if(n->index) {
			sValue *index = ast_execute(e,n->index);
			v = val_index(v,index);
			if(v)
				return val_clone(v);
			return val_createInt(0);
		}
		return val_clone(v);
	}
	return val_createInt(0);
}

void ast_printVarExpr(sVarExpr *s,uint layer) {
	UNUSED(layer);
	if(s->index) {
		printf("%s[",s->name);
		ast_printTree(s->index,layer);
		printf("]");
	}
	else
		printf("%s",s->name);
}

void ast_destroyVarExpr(sVarExpr *n) {
	ast_destroy(n->index);
	efree(n->name);
}
