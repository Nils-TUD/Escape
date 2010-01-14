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
#include <esc/fileio.h>
#include "../mem.h"
#include "node.h"
#include "command.h"

sASTNode *ast_createCommand(void) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sCommand *expr = node->data = emalloc(sizeof(sCommand));
	expr->subList = sll_create();
	/* TODO errorhandling */
	expr->runInBG = false;
	node->type = AST_COMMAND;
	return node;
}

sValue *ast_execCommand(sEnv *e,sCommand *n) {
	sSLNode *sub;
	sValue *v;
	for(sub = sll_begin(n->subList); sub != NULL; sub = sub->next) {
		v = ast_execute(e,(sASTNode*)sub->data);
		val_destroy(v);
	}
	return NULL;
}

void ast_setRunInBG(sASTNode *c,bool runInBG) {
	sCommand *cmd = (sCommand*)c->data;
	cmd->runInBG = runInBG;
}

void ast_addSubCmd(sASTNode *c,sASTNode *sub) {
	sCommand *cmd = (sCommand*)c->data;
	sll_append(cmd->subList,sub);
	/* TODO errorhandling */
}

void ast_printCommand(sCommand *s,u32 layer) {
	sSLNode *n;
	printf("%*s",layer,"");
	for(n = sll_begin(s->subList); n != NULL; n = n->next) {
		ast_printTree((sASTNode*)n->data,layer);
		if(n->next)
			printf(" | ");
	}
	if(s->runInBG)
		printf(" &");
	printf("\n");
}

void ast_destroyCommand(sCommand *c) {
	sSLNode *n;
	for(n = sll_begin(c->subList); n != NULL; n = n->next)
		ast_destroy((sASTNode*)n->data);
	sll_destroy(c->subList,false);
}
