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
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <limits.h>
#include "../mem.h"
#include "../completion.h"
#include "subcmd.h"
#include "cmdexprlist.h"
#include "conststrexpr.h"

/**
 * Wether the given node is expandable
 */
static bool ast_expandable(sASTNode *node);
/**
 * Copies <str> to <path> + <pos> and ensures that max. MAX_PATH_LEN chars are copied into <path>.
 */
static void ast_appendToPath(char *path,size_t pos,const char *str);
/**
 * Adds all pathnames that match <str> to <buf> beginning at <i> and increments <i> correspondingly
 */
static char **ast_expandPathname(char **buf,size_t *bufSize,size_t *i,char *path,char *str);

sASTNode *ast_createSubCmd(sASTNode *exprList,sASTNode *redFd,sASTNode *redirIn,sASTNode *redirOut,
		sASTNode *redirErr) {
	sASTNode *node = (sASTNode*)emalloc(sizeof(sASTNode));
	sSubCmd *expr = (sSubCmd*)emalloc(sizeof(sSubCmd));
	node->data = expr;
	expr->exprList = exprList;
	expr->redirFd = redFd;
	expr->redirIn = redirIn;
	expr->redirOut = redirOut;
	expr->redirErr = redirErr;
	node->type = AST_SUB_CMD;
	return node;
}

sValue *ast_execSubCmd(sEnv *e,sSubCmd *n) {
	size_t i;
	char *str;
	sSLNode *node,*exprNode;
	size_t exprSize;
	/* TODO the whole stuff here isn't really nice. perhaps we should move the cmdexprlist to
	 * this node or at least simply pass it to this one when executing? */
	sSLList *elist = (sSLList*)ast_execute(e,n->exprList);
	sExecSubCmd *res = (sExecSubCmd*)emalloc(sizeof(sExecSubCmd));
	res->exprCount = sll_length(elist);
	exprSize = res->exprCount + 1;
	res->exprs = (char**)emalloc(exprSize * sizeof(char*));
	exprNode = sll_begin(((sCmdExprList*)n->exprList->data)->list);
	for(i = 0, node = sll_begin(elist); node != NULL; node = node->next, exprNode = exprNode->next) {
		sASTNode *valNode = (sASTNode*)exprNode->data;
		sValue *v = (sValue*)node->data;
		str = val_getStr(v);
		if(!ast_expandable(valNode) || strchr(str,'*') == NULL) {
			/* ignore empty values if they have not been explicitly entered by "" or '' */
			if(valNode->type == AST_CONST_STR_EXPR || valNode->type == AST_DSTR_EXPR ||
					strlen(str) > 0) {
				if(i >= exprSize - 1) {
					exprSize *= 2;
					res->exprs = (char**)erealloc(res->exprs,exprSize);
				}
				res->exprs[i++] = str;
			}
		}
		else {
			char *path = (char*)emalloc(MAX_PATH_LEN + 1);
			*path = '\0';
			res->exprs = ast_expandPathname(res->exprs,&exprSize,&i,path,str);
			efree(path);
			efree(str);
		}
		val_destroy(v);
	}
	res->exprCount = i;
	res->exprs[i] = NULL;
	res->redirFd = n->redirFd;
	res->redirIn = n->redirIn;
	res->redirOut = n->redirOut;
	res->redirErr = n->redirErr;
	sll_destroy(elist,false);
	return (sValue*)res;
}

static bool ast_expandable(sASTNode *node) {
	if(node->type == AST_CONST_STR_EXPR) {
		sConstStrExpr *str = (sConstStrExpr*)node->data;
		return !str->hasQuotes;
	}
	return node->type == AST_VAR_EXPR;
}

static void ast_appendToPath(char *path,size_t pos,const char *str) {
	size_t amount = MIN(MAX_PATH_LEN - pos,strlen(str));
	strncpy(path + pos,str,amount);
	path[pos + amount] = '\0';
}

static char **ast_expandPathname(char **buf,size_t *bufSize,size_t *i,char *path,char *str) {
	bool hasStar,wasNull = false;
	size_t ipathlen = strlen(path);
	/* the basic idea is to split the pattern by '/' and append the names together until we find
	 * a '*' or are at the end. then we collect the matching items and for each item we do it
	 * recursively again if its a directory and the pattern has something left. */
	char *last = str;
	char *tok = strpbrk(str,"/");
	while(!wasNull) {
		if(tok)
			*tok = '\0';

		/* last one? */
		wasNull = tok == NULL || tok[1] == '\0';
		/* has it a star in it or is it the last one? in this case we have to collect
		 * the matching items */
		if((hasStar = strchr(last,'*') != NULL) || wasNull) {
			char apath[MAX_PATH_LEN + 1];
			DIR *dir;
			/* determine path and search-pattern */
			char *search,*pos;
			if((pos = strrchr(last,'/')) != NULL) {
				size_t x = 0;
				*pos = '\0';
				if(*path) {
					ast_appendToPath(path,ipathlen,"/");
					x++;
				}
				ast_appendToPath(path,ipathlen + x,last);
				search = pos + 1;
			}
			else {
				path[ipathlen] = '\0';
				search = last;
			}
			/* get all files in that directory */
			size_t count = cleanpath(apath,sizeof(apath),path);
			if(count > 1) {
				apath[count] = '/';
				apath[count + 1] = '\0';
			}
			if((dir = opendir(apath))) {
				size_t apathlen = strlen(apath);
				struct dirent e;
				while(readdir(dir,&e)) {
					if(strcmp(e.d_name,".") == 0 || strcmp(e.d_name,"..") == 0)
						continue;
					if(strmatch(search,e.d_name)) {
						struct stat info;
						size_t pathlen = strlen(path);
						ast_appendToPath(apath,apathlen,e.d_name);
						if(stat(apath,&info) < 0)
							continue;
						if(!wasNull) {
							if(S_ISDIR(info.st_mode)) {
								/* we need the full pattern for recursion */
								*tok = '/';
								if(pos)
									*pos = '/';
								/* append dirname */
								path[pathlen] = '/';
								ast_appendToPath(path,pathlen + 1,e.d_name);
								buf = ast_expandPathname(buf,bufSize,i,path,tok + 1);
								/* path may have been extended */
								path[pathlen] = '\0';
								/* restore pattern */
								*tok = '\0';
								if(pos)
									*pos = '\0';
							}
						}
						else {
							/* copy path and filename into a new string */
							size_t totallen = (pathlen ? pathlen + 1 : 0) + e.d_namelen + 1;
							char *duppath = (char*)emalloc(totallen + 1);
							if(pathlen) {
								strcpy(duppath,path);
								duppath[pathlen] = '/';
								strcpy(duppath + pathlen + 1,e.d_name);
							}
							else
								strcpy(duppath,e.d_name);
							/* append '/' for dirs */
							if(S_ISDIR(info.st_mode)) {
								duppath[totallen - 1] = '/';
								duppath[totallen] = '\0';
							}
							/* increase array? */
							if(*i >= *bufSize - 1) {
								*bufSize *= 2;
								buf = (char**)erealloc(buf,*bufSize * sizeof(char*));
							}
							buf[*i] = duppath;
							(*i)++;
						}
					}
				}
				closedir(dir);
			}
			/* restore pattern */
			if(tok)
				*tok = '/';
			if(pos)
				*pos = '/';
			/* break here since just recursion would go "deeper" in the pattern */
			break;
		}

		/* restore pattern and go to next if there is one */
		if(tok)
			*tok = '/';
		if(!wasNull) {
			if(hasStar)
				last = tok + 1;
			tok = strpbrk(tok + 1,"/");
		}
	}
	return buf;
}

void ast_printSubCmd(sSubCmd *s,uint layer) {
	ast_printTree(s->exprList,layer);
	ast_printTree(s->redirFd,layer);
	ast_printTree(s->redirIn,layer);
	ast_printTree(s->redirOut,layer);
	ast_printTree(s->redirErr,layer);
}

void ast_destroySubCmd(sSubCmd *n) {
	ast_destroy(n->exprList);
	ast_destroy(n->redirFd);
	ast_destroy(n->redirIn);
	ast_destroy(n->redirOut);
	ast_destroy(n->redirErr);
}
