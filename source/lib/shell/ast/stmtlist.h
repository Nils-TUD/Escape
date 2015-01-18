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

#pragma once

#include <sys/common.h>
#include <sys/sllist.h>

#include "../exec/env.h"
#include "node.h"

typedef struct {
	sSLList *list;
} sStmtList;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Creates a statement-list-node
 *
 * @return the created node
 */
sASTNode *ast_createStmtList(void);

/**
 * Executes the given node(-tree)
 *
 * @param e the environment
 * @param n the node
 * @return the value
 */
sValue *ast_execStmtList(sEnv *e,sStmtList *n);

/**
 * Adds the given statement to the list
 *
 * @param l the list
 * @param s the statement
 * @return the list
 */
sASTNode *ast_addStmt(sASTNode *l,sASTNode *s);

/**
 * Prints this statement-list
 *
 * @param s the list
 * @param layer the layer
 */
void ast_printStmtList(sStmtList *s,uint layer);

/**
 * Destroys the given statement-list (should be called from ast_destroy() only!)
 *
 * @param n the list
 */
void ast_destroyStmtList(sStmtList *n);

#if defined(__cplusplus)
}
#endif
