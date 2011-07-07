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
#include "stmtlist.h"
#include "node.h"
#include "../mem.h"

sASTNode *ast_createStmtList(void) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sStmtList *expr = node->data = emalloc(sizeof(sStmtList));
	expr->list = esll_create();
	node->type = AST_STMT_LIST;
	return node;
}

sValue *ast_execStmtList(sEnv *e,sStmtList *n) {
	sSLNode *sub;
	sValue *v;
	for(sub = sll_begin(n->list); sub != NULL; sub = sub->next) {
		v = ast_execute(e,(sASTNode*)sub->data);
		val_destroy(v);
		if(lang_isInterrupted())
			break;
	}
	return NULL;
}

sASTNode *ast_addStmt(sASTNode *l,sASTNode *s) {
	sStmtList *list = (sStmtList*)l->data;
	assert(s != NULL);
	esll_append(list->list,s);
	return l;
}

void ast_printStmtList(sStmtList *s,uint layer) {
	sSLNode *n;
	for(n = sll_begin(s->list); n != NULL; n = n->next)
		ast_printTree((sASTNode*)n->data,layer);
}

void ast_destroyStmtList(sStmtList *l) {
	sSLNode *n;
	for(n = sll_begin(l->list); n != NULL; n = n->next)
		ast_destroy((sASTNode*)n->data);
	sll_destroy(l->list,false);
}
