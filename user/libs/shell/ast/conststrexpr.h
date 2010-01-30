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

#ifndef CONSTSTREXPR_H_
#define CONSTSTREXPR_H_

#include <esc/common.h>
#include "node.h"
#include "../exec/env.h"

typedef struct {
	char *str;
} sConstStrExpr;

/**
 * Creates an const-str-node
 *
 * @param s the string (will NOT be cloned)
 * @return the created node
 */
sASTNode *ast_createConstStrExpr(char *s);

/**
 * Executes the given node(-tree)
 *
 * @param e the environment
 * @param n the node
 * @return the value
 */
sValue *ast_execConstStrExpr(sEnv *e,sConstStrExpr *n);

/**
 * Prints this expression
 *
 * @param s the expression
 * @param layer the layer
 */
void ast_printConstStrExpr(sConstStrExpr *s,u32 layer);

/**
 * Destroys the given const-str-expression (should be called from ast_destroy() only!)
 *
 * @param n the expression
 */
void ast_destroyConstStrExpr(sConstStrExpr *n);

#endif /* CONSTSTREXPR_H_ */
