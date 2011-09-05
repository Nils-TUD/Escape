/**
 * $Id: ast.h 165 2011-04-11 16:16:15Z nasmussen $
 */

#ifndef AST_H_
#define AST_H_

#include "common.h"

// AST nodes
#define AST_STMTLIST	0
#define AST_CMDSTMT		1
#define AST_EXPRLIST	2
#define AST_INTEXP		3
#define AST_FLOATEXP	4
#define AST_STREXP		5
#define AST_BINOPEXP	6
#define AST_UNOPEXP		7
#define AST_FETCHEXP	8
#define AST_RANGEEXP	9
#define AST_PCEXP		10

// binary operators
#define BINOP_ADD		0
#define BINOP_SUB		1
#define BINOP_MUL		2
#define BINOP_MULU		3
#define BINOP_DIV		4
#define BINOP_DIVU		5
#define BINOP_MOD		6
#define BINOP_MODU		7
#define BINOP_AND		8
#define BINOP_OR		9
#define BINOP_XOR		10
#define BINOP_SL		11
#define BINOP_SAR		12
#define BINOP_SR		13

// unary operators
#define UNOP_NEG		0
#define UNOP_NOT		1

// fetch-types
#define FETCH_VMEM		0
#define FETCH_VMEM1		1
#define FETCH_VMEM2		2
#define FETCH_VMEM4		3
#define FETCH_VMEM8		4
#define FETCH_PMEM		5
#define FETCH_LREG		6
#define FETCH_GREG		7
#define FETCH_SREG		8
#define FETCH_DREG		9

// origins
#define ORG_VMEM		0
#define ORG_VMEM1		1
#define ORG_VMEM2		2
#define ORG_VMEM4		3
#define ORG_VMEM8		4
#define ORG_PMEM		5
#define ORG_LREG		6
#define ORG_GREG		7
#define ORG_SREG		8
#define ORG_DREG		9
#define ORG_PC			10
#define ORG_EXPR		11

// range-types
#define RANGE_FROMTO	0
#define RANGE_FROMCNT	1

typedef struct sASTNode {
	// type of node (AST_*)
	int type;
	// data for the node, depending on the type
	union {
		struct {
			bool isEmpty;
			struct sASTNode *head;
			struct sASTNode *tail;
		} stmtList;
		struct {
			char *name;
			struct sASTNode *args;
		} cmdStmt;
		struct {
			bool isEmpty;
			struct sASTNode *head;
			struct sASTNode *tail;
		} exprList;
		struct {
			octa value;
		} intExp;
		struct {
			double value;
		} floatExp;
		struct {
			char *value;
		} strExp;
		struct {
			int op;
			struct sASTNode *left;
			struct sASTNode *right;
		} binOpExp;
		struct {
			int op;
			struct sASTNode *expr;
		} unOpExp;
		struct {
			int op;
			struct sASTNode *expr;
		} fetchExp;
		struct {
			int op;
			struct sASTNode *left;
			struct sASTNode *right;
		} rangeExp;
	} d;
} sASTNode;

/**
 * Creates an AST-node with the corresponding parameters
 */
sASTNode *ast_createStmtList(sASTNode *head,sASTNode *tail);
sASTNode *ast_createCmdStmt(char *name,sASTNode *args);
sASTNode *ast_createExprList(sASTNode *head,sASTNode *tail);
sASTNode *ast_createIntExpr(octa value);
sASTNode *ast_createFloatExpr(double value);
sASTNode *ast_createStrExpr(char *value);
sASTNode *ast_createPCExpr(void);
sASTNode *ast_createBinOpExpr(int op,sASTNode *left,sASTNode *right);
sASTNode *ast_createUnOpExpr(int op,sASTNode *expr);
sASTNode *ast_createFetchExpr(int op,sASTNode *expr);
sASTNode *ast_createRangeExpr(int op,sASTNode *left,sASTNode *right);

/**
 * Clones the given AST-tree (recursively)
 *
 * @param node the tree
 * @return the cloned tree
 */
sASTNode *ast_clone(const sASTNode *node);

/**
 * Writes the string-representation of the given AST-tree into <str>
 *
 * @param node the tree
 * @param str the string to write to
 * @param size the size of <str>
 * @return the number of chars written to <str>
 */
size_t ast_toString(const sASTNode *node,char *str,size_t size);

/**
 * Destroyes the given AST-tree, i.e. it free's all allocated memory
 *
 * @param node the tree
 */
void ast_destroy(sASTNode *node);

#endif /* AST_H_ */
