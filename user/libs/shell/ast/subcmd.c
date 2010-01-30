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
#include "../mem.h"
#include "subcmd.h"

sASTNode *ast_createSubCmd(sASTNode *exprList,sASTNode *redirFd,sASTNode *redirIn,sASTNode *redirOut) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sSubCmd *expr = node->data = emalloc(sizeof(sSubCmd));
	expr->exprList = exprList;
	expr->redirFd = redirFd;
	expr->redirIn = redirIn;
	expr->redirOut = redirOut;
	node->type = AST_SUB_CMD;
	return node;
}

sValue *ast_execSubCmd(sEnv *e,sSubCmd *n) {
	u32 i;
	sSLNode *node;
	sSLList *elist = (sSLList*)ast_execute(e,n->exprList);
	sExecSubCmd *res = emalloc(sizeof(sExecSubCmd));
	res->exprCount = sll_length(elist);
	res->exprs = (char**)emalloc((res->exprCount + 1) * sizeof(char*));
	for(i = 0, node = sll_begin(elist); node != NULL; i++, node = node->next) {
		sValue *v = (sValue*)node->data;
		res->exprs[i] = val_getStr(v);
		val_destroy(v);
	}
	res->exprs[i] = NULL;
	res->redirFd = n->redirFd;
	res->redirIn = n->redirIn;
	res->redirOut = n->redirOut;
	sll_destroy(elist,false);
	return (sValue*)res;
}

void ast_printSubCmd(sSubCmd *s,u32 layer) {
	ast_printTree(s->exprList,layer);
	ast_printTree(s->redirFd,layer);
	ast_printTree(s->redirIn,layer);
	ast_printTree(s->redirOut,layer);
}

void ast_destroySubCmd(sSubCmd *n) {
	ast_destroy(n->exprList);
	ast_destroy(n->redirFd);
	ast_destroy(n->redirIn);
	ast_destroy(n->redirOut);
}
