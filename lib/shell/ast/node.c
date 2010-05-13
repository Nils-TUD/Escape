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
#include "node.h"
#include "assignexpr.h"
#include "binaryopexpr.h"
#include "cmpexpr.h"
#include "conststrexpr.h"
#include "ifstmt.h"
#include "stmtlist.h"
#include "intexpr.h"
#include "unaryopexpr.h"
#include "varexpr.h"
#include "command.h"
#include "cmdexprlist.h"
#include "subcmd.h"
#include "redirfd.h"
#include "redirfile.h"
#include "forstmt.h"
#include "exprstmt.h"
#include "dstrexpr.h"
#include "whilestmt.h"
#include "functionstmt.h"
#include "../mem.h"

void ast_printTree(sASTNode *n,u32 layer) {
	switch(n->type) {
		case AST_ASSIGN_STMT:
			ast_printAssignExpr((sAssignExpr*)n->data,layer);
			break;
		case AST_BINARY_OP_EXPR:
			ast_printBinOpExpr((sBinaryOpExpr*)n->data,layer);
			break;
		case AST_CMP_OP_EXPR:
			ast_printCmpExpr((sCmpExpr*)n->data,layer);
			break;
		case AST_CONST_STR_EXPR:
			ast_printConstStrExpr((sConstStrExpr*)n->data,layer);
			break;
		case AST_IF_STMT:
			ast_printIfStmt((sIfStmt*)n->data,layer);
			break;
		case AST_STMT_LIST:
			ast_printStmtList((sStmtList*)n->data,layer);
			break;
		case AST_INT_EXPR:
			ast_printIntExpr((sIntExpr*)n->data,layer);
			break;
		case AST_UNARY_OP_EXPR:
			ast_printUnaryOpExpr((sUnaryOpExpr*)n->data,layer);
			break;
		case AST_VAR_EXPR:
			ast_printVarExpr((sVarExpr*)n->data,layer);
			break;
		case AST_COMMAND:
			ast_printCommand((sCommand*)n->data,layer);
			break;
		case AST_CMDEXPR_LIST:
			ast_printCmdExprList((sCmdExprList*)n->data,layer);
			break;
		case AST_SUB_CMD:
			ast_printSubCmd((sSubCmd*)n->data,layer);
			break;
		case AST_REDIR_FD:
			ast_printRedirFd((sRedirFd*)n->data,layer);
			break;
		case AST_REDIR_FILE:
			ast_printRedirFile((sRedirFile*)n->data,layer);
			break;
		case AST_FOR_STMT:
			ast_printForStmt((sForStmt*)n->data,layer);
			break;
		case AST_EXPR_STMT:
			ast_printExprStmt((sExprStmt*)n->data,layer);
			break;
		case AST_DSTR_EXPR:
			ast_printDStrExpr((sDStrExpr*)n->data,layer);
			break;
		case AST_WHILE_STMT:
			ast_printWhileStmt((sWhileStmt*)n->data,layer);
			break;
		case AST_FUNC_STMT:
			ast_printFunctionStmt((sFunctionStmt*)n->data,layer);
			break;
	}
}

sValue *ast_execute(sEnv *e,sASTNode *n) {
	switch(n->type) {
		case AST_ASSIGN_STMT:
			return ast_execAssignExpr(e,(sAssignExpr*)n->data);
		case AST_BINARY_OP_EXPR:
			return ast_execBinOpExpr(e,(sBinaryOpExpr*)n->data);
		case AST_CMP_OP_EXPR:
			return ast_execCmpExpr(e,(sCmpExpr*)n->data);
		case AST_CONST_STR_EXPR:
			return ast_execConstStrExpr(e,(sConstStrExpr*)n->data);
		case AST_IF_STMT:
			return ast_execIfStmt(e,(sIfStmt*)n->data);
		case AST_STMT_LIST:
			return ast_execStmtList(e,(sStmtList*)n->data);
		case AST_INT_EXPR:
			return ast_execIntExpr(e,(sIntExpr*)n->data);
		case AST_UNARY_OP_EXPR:
			return ast_execUnaryOpExpr(e,(sUnaryOpExpr*)n->data);
		case AST_VAR_EXPR:
			return ast_execVarExpr(e,(sVarExpr*)n->data);
		case AST_COMMAND:
			return ast_execCommand(e,(sCommand*)n->data);
		case AST_CMDEXPR_LIST:
			return ast_execCmdExprList(e,(sCmdExprList*)n->data);
		case AST_SUB_CMD:
			return ast_execSubCmd(e,(sSubCmd*)n->data);
		case AST_FOR_STMT:
			return ast_execForStmt(e,(sForStmt*)n->data);
		case AST_EXPR_STMT:
			return ast_execExprStmt(e,(sExprStmt*)n->data);
		case AST_DSTR_EXPR:
			return ast_execDStrExpr(e,(sDStrExpr*)n->data);
		case AST_WHILE_STMT:
			return ast_execWhileStmt(e,(sWhileStmt*)n->data);
		case AST_FUNC_STMT:
			return ast_execFunctionStmt(e,(sFunctionStmt*)n->data);
	}
	/* never reached */
	return NULL;
}

void ast_destroy(sASTNode *n) {
	if(n == NULL)
		return;
	switch(n->type) {
		case AST_ASSIGN_STMT:
			ast_destroyAssignExpr((sAssignExpr*)n->data);
			break;
		case AST_BINARY_OP_EXPR:
			ast_destroyBinOpExpr((sBinaryOpExpr*)n->data);
			break;
		case AST_CMP_OP_EXPR:
			ast_destroyCmpExpr((sCmpExpr*)n->data);
			break;
		case AST_CONST_STR_EXPR:
			ast_destroyConstStrExpr((sConstStrExpr*)n->data);
			break;
		case AST_IF_STMT:
			ast_destroyIfStmt((sIfStmt*)n->data);
			break;
		case AST_STMT_LIST:
			ast_destroyStmtList((sStmtList*)n->data);
			break;
		case AST_INT_EXPR:
			ast_destroyIntExpr((sIntExpr*)n->data);
			break;
		case AST_UNARY_OP_EXPR:
			ast_destroyUnaryOpExpr((sUnaryOpExpr*)n->data);
			break;
		case AST_VAR_EXPR:
			ast_destroyVarExpr((sVarExpr*)n->data);
			break;
		case AST_COMMAND:
			ast_destroyCommand((sCommand*)n->data);
			break;
		case AST_CMDEXPR_LIST:
			ast_destroyCmdExprList((sCmdExprList*)n->data);
			break;
		case AST_SUB_CMD:
			ast_destroySubCmd((sSubCmd*)n->data);
			break;
		case AST_REDIR_FD:
			ast_destroyRedirFd((sRedirFd*)n->data);
			break;
		case AST_REDIR_FILE:
			ast_destroyRedirFile((sRedirFile*)n->data);
			break;
		case AST_FOR_STMT:
			ast_destroyForStmt((sForStmt*)n->data);
			break;
		case AST_EXPR_STMT:
			ast_destroyExprStmt((sExprStmt*)n->data);
			break;
		case AST_DSTR_EXPR:
			ast_destroyDStrExpr((sDStrExpr*)n->data);
			break;
		case AST_WHILE_STMT:
			ast_destroyWhileStmt((sWhileStmt*)n->data);
			break;
		case AST_FUNC_STMT:
			ast_destroyFunctionStmt((sFunctionStmt*)n->data);
			break;
	}
	efree(n->data);
	efree(n);
}
