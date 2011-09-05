/**
 * $Id: ast.c 165 2011-04-11 16:16:15Z nasmussen $
 */

#include <string.h>
#include <assert.h>

#include "common.h"
#include "cli/lang/ast.h"
#include "mmix/io.h"
#include "mmix/mem.h"

static void ast_addToStr(char **str,size_t *size,const char *fmt,...);
static void ast_doToString(const sASTNode *node,char **str,size_t *size,int indent);
static sASTNode *ast_createNode(int type);

static const char *binOps[] = {
	"+","-","s*","*","s/","/","s%","%","&","|","^","<<",">>>",">>"
};
static const char *unOps[] = {
	"-","~"
};
static const char *fetchOps[] = {
	"M","M1","M2","M4","M8","m","l","g","sp","$"
};
static const char *rangeOps[] = {
	"..",":"
};

sASTNode *ast_createStmtList(sASTNode *head,sASTNode *tail) {
	sASTNode *n = ast_createNode(AST_STMTLIST);
	n->d.stmtList.isEmpty = tail == NULL;
	n->d.stmtList.head = head;
	n->d.stmtList.tail = tail;
	return n;
}

sASTNode *ast_createCmdStmt(char *name,sASTNode *args) {
	sASTNode *n = ast_createNode(AST_CMDSTMT);
	n->d.cmdStmt.name = name;
	n->d.cmdStmt.args = args;
	return n;
}

sASTNode *ast_createExprList(sASTNode *head,sASTNode *tail) {
	sASTNode *n = ast_createNode(AST_EXPRLIST);
	n->d.exprList.isEmpty = tail == NULL;
	n->d.exprList.head = head;
	n->d.exprList.tail = tail;
	return n;
}

sASTNode *ast_createIntExpr(octa value) {
	sASTNode *n = ast_createNode(AST_INTEXP);
	n->d.intExp.value = value;
	return n;
}

sASTNode *ast_createFloatExpr(double value) {
	sASTNode *n = ast_createNode(AST_FLOATEXP);
	n->d.floatExp.value = value;
	return n;
}

sASTNode *ast_createStrExpr(char *value) {
	sASTNode *n = ast_createNode(AST_STREXP);
	n->d.strExp.value = value;
	return n;
}

sASTNode *ast_createPCExpr(void) {
	return ast_createNode(AST_PCEXP);
}

sASTNode *ast_createBinOpExpr(int op,sASTNode *left,sASTNode *right) {
	sASTNode *n = ast_createNode(AST_BINOPEXP);
	n->d.binOpExp.op = op;
	n->d.binOpExp.left = left;
	n->d.binOpExp.right = right;
	return n;
}

sASTNode *ast_createUnOpExpr(int op,sASTNode *expr) {
	sASTNode *n = ast_createNode(AST_UNOPEXP);
	n->d.unOpExp.op = op;
	n->d.unOpExp.expr = expr;
	return n;
}

sASTNode *ast_createFetchExpr(int op,sASTNode *expr) {
	sASTNode *n = ast_createNode(AST_FETCHEXP);
	n->d.fetchExp.op = op;
	n->d.fetchExp.expr = expr;
	return n;
}

sASTNode *ast_createRangeExpr(int op,sASTNode *left,sASTNode *right) {
	sASTNode *n = ast_createNode(AST_RANGEEXP);
	n->d.rangeExp.op = op;
	n->d.rangeExp.left = left;
	n->d.rangeExp.right = right;
	return n;
}

sASTNode *ast_clone(const sASTNode *node) {
	if(node == NULL)
		return NULL;

	sASTNode *n = (sASTNode*)mem_alloc(sizeof(sASTNode));
	n->type = node->type;
	switch(node->type) {
		case AST_STMTLIST:
			n->d.stmtList.head = ast_clone(node->d.stmtList.head);
			n->d.stmtList.tail = ast_clone(node->d.stmtList.tail);
			n->d.stmtList.isEmpty = node->d.stmtList.isEmpty;
			break;
		case AST_CMDSTMT:
			n->d.cmdStmt.args = ast_clone(node->d.cmdStmt.args);
			n->d.cmdStmt.name = (char*)mem_alloc(strlen(node->d.cmdStmt.name) + 1);
			strcpy(n->d.cmdStmt.name,node->d.cmdStmt.name);
			break;
		case AST_INTEXP:
			n->d.intExp.value = node->d.intExp.value;
			break;
		case AST_FLOATEXP:
			n->d.floatExp.value = node->d.floatExp.value;
			break;
		case AST_STREXP:
			n->d.strExp.value = (char*)mem_alloc(strlen(node->d.strExp.value) + 1);
			strcpy(n->d.strExp.value,node->d.strExp.value);
			break;
		case AST_PCEXP:
			break;
		case AST_BINOPEXP:
			n->d.binOpExp.left = ast_clone(node->d.binOpExp.left);
			n->d.binOpExp.op = node->d.binOpExp.op;
			n->d.binOpExp.right = ast_clone(node->d.binOpExp.right);
			break;
		case AST_UNOPEXP:
			n->d.unOpExp.expr = ast_clone(node->d.unOpExp.expr);
			n->d.unOpExp.op = node->d.unOpExp.op;
			break;
		case AST_FETCHEXP:
			n->d.fetchExp.expr = ast_clone(node->d.fetchExp.expr);
			n->d.fetchExp.op = node->d.fetchExp.op;
			break;
		case AST_RANGEEXP:
			n->d.rangeExp.left = ast_clone(node->d.rangeExp.left);
			n->d.rangeExp.op = node->d.rangeExp.op;
			n->d.rangeExp.right = ast_clone(node->d.rangeExp.right);
			break;
		default:
			assert(false);
			break;
	}
	return n;
}

void ast_destroy(sASTNode *node) {
	sASTNode *list;
	switch(node->type) {
		case AST_STMTLIST:
			list = node;
			while(!list->d.stmtList.isEmpty) {
				sASTNode *tmp = list->d.stmtList.tail;
				ast_destroy(list->d.stmtList.head);
				list = tmp;
			}
			break;
		case AST_CMDSTMT:
			mem_free(node->d.cmdStmt.name);
			ast_destroy(node->d.cmdStmt.args);
			break;
		case AST_EXPRLIST:
			list = node;
			while(!list->d.exprList.isEmpty) {
				sASTNode *tmp = list->d.exprList.tail;
				ast_destroy(list->d.exprList.head);
				list = tmp;
			}
			break;
		case AST_INTEXP:
			// nothing to do
			break;
		case AST_FLOATEXP:
			// nothing to do
			break;
		case AST_STREXP:
			mem_free(node->d.strExp.value);
			break;
		case AST_PCEXP:
			// nothing to do
			break;
		case AST_BINOPEXP:
			ast_destroy(node->d.binOpExp.left);
			ast_destroy(node->d.binOpExp.right);
			break;
		case AST_UNOPEXP:
			ast_destroy(node->d.unOpExp.expr);
			break;
		case AST_FETCHEXP:
			ast_destroy(node->d.fetchExp.expr);
			break;
		case AST_RANGEEXP:
			ast_destroy(node->d.rangeExp.left);
			ast_destroy(node->d.rangeExp.right);
			break;
		default:
			assert(false);
			break;
	}
	mem_free(node);
}

size_t ast_toString(const sASTNode *node,char *str,size_t size) {
	size_t rem = size;
	ast_doToString(node,&str,&rem,0);
	return size - rem;
}

static void ast_addToStr(char **str,size_t *size,const char *fmt,...) {
	va_list ap;
	va_start(ap,fmt);
	int res = mvsnprintf(*str,*size,fmt,ap);
	*str += res;
	*size -= res;
	va_end(ap);
}

static void ast_doToString(const sASTNode *node,char **str,size_t *size,int indent) {
	const sASTNode *list;
	switch(node->type) {
		case AST_STMTLIST:
			list = node;
			ast_addToStr(str,size,"%*sStmtList[\n",indent * 2,"");
			while(!list->d.stmtList.isEmpty) {
				ast_doToString(list->d.stmtList.head,str,size,indent + 1);
				list = list->d.stmtList.tail;
			}
			ast_addToStr(str,size,"%*s]\n",indent * 2,"");
			break;
		case AST_CMDSTMT:
			ast_addToStr(str,size,"%*s%s ",indent * 2,"",node->d.cmdStmt.name);
			ast_doToString(node->d.cmdStmt.args,str,size,indent);
			ast_addToStr(str,size,"\n");
			break;
		case AST_EXPRLIST:
			list = node;
			while(!list->d.exprList.isEmpty) {
				ast_doToString(list->d.exprList.head,str,size,indent);
				list = list->d.exprList.tail;
				if(!list->d.exprList.isEmpty)
					ast_addToStr(str,size," ");
			}
			break;
		case AST_INTEXP:
			ast_addToStr(str,size,"#%OX",node->d.intExp.value);
			break;
		case AST_FLOATEXP:
			ast_addToStr(str,size,"%g",node->d.floatExp.value);
			break;
		case AST_STREXP:
			ast_addToStr(str,size,"%s",node->d.strExp.value);
			break;
		case AST_PCEXP:
			ast_addToStr(str,size,"@");
			break;
		case AST_BINOPEXP:
			ast_addToStr(str,size,"(");
			ast_doToString(node->d.binOpExp.left,str,size,indent);
			ast_addToStr(str,size,"%s",binOps[node->d.binOpExp.op]);
			ast_doToString(node->d.binOpExp.right,str,size,indent);
			ast_addToStr(str,size,")");
			break;
		case AST_UNOPEXP:
			ast_addToStr(str,size,"(%s",unOps[node->d.unOpExp.op]);
			ast_doToString(node->d.unOpExp.expr,str,size,indent);
			ast_addToStr(str,size,")");
			break;
		case AST_FETCHEXP:
			ast_addToStr(str,size,"%s[",fetchOps[node->d.fetchExp.op]);
			ast_doToString(node->d.fetchExp.expr,str,size,indent);
			ast_addToStr(str,size,"]");
			break;
		case AST_RANGEEXP:
			ast_doToString(node->d.rangeExp.left,str,size,indent);
			ast_addToStr(str,size,"%s",rangeOps[node->d.rangeExp.op]);
			ast_doToString(node->d.rangeExp.right,str,size,indent);
			break;
		default:
			assert(false);
			break;
	}
}

static sASTNode *ast_createNode(int type) {
	sASTNode *n = mem_alloc(sizeof(sASTNode));
	n->type = type;
	return n;
}
