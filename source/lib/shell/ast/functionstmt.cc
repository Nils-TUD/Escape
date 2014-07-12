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
#include "node.h"
#include "functionstmt.h"
#include "../mem.h"

sASTNode *ast_createFunctionStmt(char *name,sASTNode *stmts) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sFunctionStmt *stmt = (sFunctionStmt*)emalloc(sizeof(sFunctionStmt));
	node->data = stmt;
	/* no clone necessary here because we've already cloned it in the scanner */
	stmt->name = name;
	stmt->stmts = stmts;
	stmt->env = NULL;
	node->type = AST_FUNC_STMT;
	return node;
}

sValue *ast_execFunctionStmt(sEnv *e,sFunctionStmt *n) {
	/* create a copy which we can keep in the env */
	sFunctionStmt *cpy = (sFunctionStmt*)emalloc(sizeof(sFunctionStmt));
	/* store env because we want to have static binding */
	cpy->env = e;
	cpy->name = n->name;
	cpy->stmts = n->stmts;
	env_set(e,n->name,val_createFunc(cpy));
	return NULL;
}

int ast_callFunction(sFunctionStmt *n,int argc,const char **argv) {
	sValue *v;
	sEnv *ne = env_create(n->env);
	env_addArgs(ne,argc,argv);
	v = ast_execute(ne,n->stmts);
	val_destroy(v);
	env_destroy(ne);
	return 0;
}

void ast_printFunctionStmt(sFunctionStmt *n,uint layer) {
	printf("%*sfunction %s begin\n",layer,"",n->name);
	ast_printTree(n->stmts,layer + 1);
	printf("%*send\n",layer,"");
}

void ast_destroyFunctionStmt(A_UNUSED sFunctionStmt *n) {
	/* do nothing because we want to keep the function-AST (in the env) */
}

void ast_killFunctionStmt(sFunctionStmt *n) {
	ast_destroy(n->stmts);
	efree(n->name);
	efree(n);
}
