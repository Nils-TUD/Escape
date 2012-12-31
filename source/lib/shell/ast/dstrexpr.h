/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#pragma once

#include <esc/common.h>
#include <esc/sllist.h>
#include "node.h"
#include "../exec/env.h"

typedef struct {
	sSLList *list;
} sDStrExpr;

/**
 * Creates an dynamic-string-expression
 *
 * @return the created node
 */
sASTNode *ast_createDStrExpr(void);

/**
 * Adds the given component to the dynamic-string
 *
 * @param n the dyn-string
 * @param expr the expression
 */
void ast_addDStrComp(sASTNode *n,sASTNode *expr);

/**
 * Executes the given node(-tree)
 *
 * @param e the environment
 * @param n the node
 * @return the value
 */
sValue *ast_execDStrExpr(sEnv *e,sDStrExpr *n);

/**
 * Prints this expression
 *
 * @param s the expression
 * @param layer the layer
 */
void ast_printDStrExpr(sDStrExpr *s,uint layer);

/**
 * Destroys the given expression (should be called from ast_destroy() only!)
 *
 * @param n the expression
 */
void ast_destroyDStrExpr(sDStrExpr *n);
