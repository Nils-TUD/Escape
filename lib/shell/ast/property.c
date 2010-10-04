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
#include <stdio.h>
#include "property.h"
#include "node.h"
#include "../mem.h"
#include "../exec/value.h"

sASTNode *ast_createProperty(sASTNode *expr,char *name,sASTNode *args) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sProperty *p = node->data = emalloc(sizeof(sProperty));
	p->expr = expr;
	p->args = args;
	/* no clone here since we've already cloned it in the scanner */
	p->name = name;
	node->type = AST_PROPERTY;
	return node;
}

sValue *ast_execProperty(sEnv *e,sProperty *n) {
	sValue *res;
	/* TODO there is a better way... */
	sValue *v = ast_execute(e,n->expr);
	if(strcmp(n->name,"len") == 0)
		res = val_createInt(val_len(v));
	else if(strcmp(n->name,"tos") == 0)
		res = val_tos(v);
	else if(strcmp(n->name,"toa") == 0)
		res = val_toa(v);
	else if(strcmp(n->name,"sub") == 0) {
		sValue *args,*start,*count;
		assert(n->args);
		args = ast_execute(e,n->args);
		start = val_createInt(0);
		count = val_createInt(1);
		res = val_sub(v,val_index(args,start),val_index(args,count));
		val_destroy(args);
		val_destroy(start);
		val_destroy(count);
	}
	else
		res = val_createInt(0);
	val_destroy(v);
	return res;
}

void ast_printProperty(sProperty *s,uint layer) {
	ast_printTree(s->expr,layer);
	printf(".%s",s->name);
	if(s->args) {
		printf("(");
		ast_printTree(s->args,layer);
		printf(")");
	}
}

void ast_destroyProperty(sProperty *n) {
	ast_destroy(n->expr);
	ast_destroy(n->args);
	efree(n->name);
}
