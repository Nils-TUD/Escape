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
#include "assignstmt.h"
#include "binaryopexpr.h"
#include "cmpexpr.h"
#include "conststrexpr.h"
#include "ifstmt.h"
#include "stmtlist.h"
#include "intexpr.h"
#include "unaryopexpr.h"
#include "varexpr.h"
#include "../mem.h"

void ast_printTree(sASTNode *n,u32 layer) {
	switch(n->type) {
		case AST_ASSIGN_STMT:
			ast_printAssignStmt((sAssignStmt*)n->data,layer);
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
	}
}

void ast_destroy(sASTNode *n) {
	switch(n->type) {
		case AST_ASSIGN_STMT:
			ast_destroyAssignStmt((sAssignStmt*)n->data);
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
	}
	efree(n->data);
	efree(n);
}
