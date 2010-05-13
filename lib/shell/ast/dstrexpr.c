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
#include "dstrexpr.h"
#include "node.h"
#include "../mem.h"

#define STR_BUF_SIZE	16

sASTNode *ast_createDStrExpr(void) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sDStrExpr *res = node->data = emalloc(sizeof(sDStrExpr));
	res->list = esll_create();
	node->type = AST_DSTR_EXPR;
	return node;
}

void ast_addDStrComp(sASTNode *n,sASTNode *expr) {
	sDStrExpr *dstr = (sDStrExpr*)n->data;
	esll_append(dstr->list,expr);
}

sValue *ast_execDStrExpr(sEnv *e,sDStrExpr *n) {
	sSLNode *node;
	sValue *v;
	u32 total = 0,len;
	char *part;
	char *buf = NULL;
	if(sll_length(n->list) == 0)
		buf = estrdup("");
	else {
		for(node = sll_begin(n->list); node != NULL; node = node->next) {
			v = ast_execute(e,(sASTNode*)node->data);
			part = val_getStr(v);
			len = strlen(part);
			if(!buf)
				buf = emalloc(len + 1);
			else
				buf = erealloc(buf,total + len + 1);
			strcpy(buf + total,part);
			total += len;
			efree(part);
			val_destroy(v);
		}
	}
	return val_createStr(buf);
}

void ast_printDStrExpr(sDStrExpr *s,u32 layer) {
	sSLNode *n;
	printf("\"");
	for(n = sll_begin(s->list); n != NULL; n = n->next)
		ast_printTree((sASTNode*)n->data,layer);
	printf("\"");
}

void ast_destroyDStrExpr(sDStrExpr *n) {
	sSLNode *node;
	for(node = sll_begin(n->list); node != NULL; node = node->next)
		ast_destroy((sASTNode*)node->data);
	sll_destroy(n->list,false);
}
