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
#include "cmdexprlist.h"
#include "node.h"
#include "../mem.h"

sASTNode *ast_createCmdExprList(void) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sCmdExprList *expr = node->data = emalloc(sizeof(sCmdExprList));
	expr->list = esll_create();
	node->type = AST_CMDEXPR_LIST;
	return node;
}

sValue *ast_execCmdExprList(sEnv *e,sCmdExprList *n) {
	sSLNode *sub;
	sSLList *res = esll_create();
	for(sub = sll_begin(n->list); sub != NULL; sub = sub->next) {
		sValue *v = ast_execute(e,(sASTNode*)sub->data);
		esll_append(res,v);
	}
	return (sValue*)res;
}

sASTNode *ast_addCmdExpr(sASTNode *l,sASTNode *s) {
	sCmdExprList *list = (sCmdExprList*)l->data;
	esll_append(list->list,s);
	return l;
}

void ast_printCmdExprList(sCmdExprList *s,uint layer) {
	sSLNode *n;
	for(n = sll_begin(s->list); n != NULL; n = n->next) {
		ast_printTree((sASTNode*)n->data,layer);
		if(n->next != NULL)
			printf(" ");
	}
}

void ast_destroyCmdExprList(sCmdExprList *l) {
	sSLNode *n;
	for(n = sll_begin(l->list); n != NULL; n = n->next)
		ast_destroy((sASTNode*)n->data);
	sll_destroy(l->list,false);
}
