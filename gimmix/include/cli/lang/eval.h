/**
 * $Id: eval.h 171 2011-04-14 13:18:30Z nasmussen $
 */

#ifndef EVAL_H_
#define EVAL_H_

#include "common.h"
#include "cli/lang/ast.h"

typedef struct {
	int type;		// ARGVAL_INT or ARGVAL_FLOAT or ARGVAL_STR
	int origin;		// ORG_*
	octa location;	// if not ORG_EXPR, the memory-offset or register-number
	union {
		octa integer;
		double floating;
		const char *str;
	} d;
} sCmdArg;

/**
 * Evaluates the given AST-node with given offset, i.e. it will go through the tree and calculate
 * the value of the node.
 *
 * @param node the AST-node
 * @param offset the current offset (needed for ranges)
 * @param finished will be set to false if a range is not yet finished (i.e. you should set it to
 *  true first)
 * @return the value of the node
 */
sCmdArg eval_get(const sASTNode *node,octa offset,bool *finished);

#endif /* EVAL_H_ */
