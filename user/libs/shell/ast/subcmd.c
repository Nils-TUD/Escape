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
#include <fsinterface.h>
#include "../mem.h"
#include "../completion.h"
#include "subcmd.h"
#include "cmdexprlist.h"
#include "conststrexpr.h"

/**
 * Wether the given node is expandable
 */
static bool ast_expandable(sASTNode *node);
/**
 * Adds all pathnames that match <str> to <buf> beginning at <i> and increments <i> correspondingly
 */
static char **ast_expandPathname(char **buf,u32 *bufSize,u32 *i,const char *str);

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
	char *str;
	sSLNode *node,*exprNode;
	u32 exprSize;
	/* TODO the whole stuff here isn't really nice. perhaps we should move the cmdexprlist to
	 * this node or at least simply pass it to this one when executing? */
	sSLList *elist = (sSLList*)ast_execute(e,n->exprList);
	sExecSubCmd *res = emalloc(sizeof(sExecSubCmd));
	res->exprCount = sll_length(elist);
	exprSize = res->exprCount + 1;
	res->exprs = (char**)emalloc(exprSize * sizeof(char*));
	exprNode = sll_begin(((sCmdExprList*)n->exprList->data)->list);
	for(i = 0, node = sll_begin(elist); node != NULL; node = node->next, exprNode = exprNode->next) {
		sASTNode *valNode = (sASTNode*)exprNode->data;
		sValue *v = (sValue*)node->data;
		str = val_getStr(v);
		if(!ast_expandable(valNode) || strchr(str,'*') == NULL) {
			if(i >= exprSize - 1) {
				exprSize *= 2;
				res->exprs = erealloc(res->exprs,exprSize);
			}
			res->exprs[i++] = str;
		}
		else {
			res->exprs = ast_expandPathname(res->exprs,&exprSize,&i,str);
			efree(str);
		}
		val_destroy(v);
	}
	res->exprCount = i;
	res->exprs[i] = NULL;
	res->redirFd = n->redirFd;
	res->redirIn = n->redirIn;
	res->redirOut = n->redirOut;
	sll_destroy(elist,false);
	return (sValue*)res;
}

static bool ast_expandable(sASTNode *node) {
	if(node->type == AST_CONST_STR_EXPR) {
		sConstStrExpr *str = (sConstStrExpr*)node->data;
		return !str->hasQuotes;
	}
	return node->type == AST_VAR_EXPR;
}

static char **ast_expandPathname(char **buf,u32 *bufSize,u32 *i,const char *str) {
	sShellCmd **cmds = compl_get((char*)"",0,0,false,false);
	sShellCmd **cmd;
	cmd = cmds;
	while(*cmd != NULL) {
		if(strmatch(str,(*cmd)->name)) {
			u32 namelen = strlen((*cmd)->name);
			char *dup = (char*)emalloc(namelen + 2);
			strcpy(dup,(*cmd)->name);
			if(MODE_IS_DIR((*cmd)->mode)) {
				dup[namelen] = '/';
				dup[namelen + 1] = '\0';
			}
			if(*i >= *bufSize - 1) {
				*bufSize *= 2;
				buf = erealloc(buf,*bufSize * sizeof(char*));
			}
			buf[*i] = dup;
			(*i)++;
		}
		cmd++;
	}
	compl_free(cmds);
	return buf;
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
