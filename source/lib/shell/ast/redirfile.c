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

#include <esc/common.h>
#include <stdio.h>
#include "../mem.h"
#include "redirfile.h"
#include "node.h"

sASTNode *ast_createRedirFile(sASTNode *expr,uchar type) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sRedirFile *res = node->data = emalloc(sizeof(sRedirFile));
	res->expr = expr;
	res->type = type;
	node->type = AST_REDIR_FILE;
	return node;
}

void ast_printRedirFile(sRedirFile *s,A_UNUSED uint layer) {
	if(s->expr) {
		switch(s->type) {
			case REDIR_INFILE:
				printf(" <");
				break;
			case REDIR_OUTCREATE:
				printf(" >");
				break;
			case REDIR_OUTAPPEND:
				printf(" >>");
				break;
		}
		ast_printTree(s->expr,layer);
	}
}

void ast_destroyRedirFile(sRedirFile *n) {
	if(n->expr)
		ast_destroy(n->expr);
}
