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
#include <stdlib.h>
#include "value.h"
#include "../mem.h"
#include "../ast/cmpexpr.h"
#include "../ast/functionstmt.h"

sValue *val_createInt(tIntType i) {
	sValue *res = (sValue*)emalloc(sizeof(sValue));
	res->type = TYPE_INT;
	res->v.intval = i;
	return res;
}

sValue *val_createStr(const char *s) {
	sValue *res = (sValue*)emalloc(sizeof(sValue));
	res->type = TYPE_STR;
	res->v.strval = estrdup(s);
	return res;
}

sValue *val_createFunc(void *func) {
	sValue *res = (sValue*)emalloc(sizeof(sValue));
	res->type = TYPE_FUNC;
	res->v.func = func;
	return res;
}

sValue *val_clone(sValue *v) {
	sValue *res = (sValue*)emalloc(sizeof(sValue));
	res->type = v->type;
	switch(v->type) {
		case TYPE_INT:
			res->v.intval = v->v.intval;
			break;
		case TYPE_STR:
			res->v.strval = estrdup(v->v.strval);
			break;
		case TYPE_FUNC:
			res->v.func = v->v.func;
			break;
	}
	return res;
}

void val_destroy(sValue *v) {
	if(v && v->type == TYPE_FUNC)
		ast_killFunctionStmt((sFunctionStmt*)v->v.func);
	else if(v && v->type == TYPE_STR)
		efree(v->v.strval);
	efree(v);
}

bool val_isTrue(sValue *v) {
	switch(v->type) {
		case TYPE_INT:
			return val_getInt(v) != 0;
		case TYPE_STR: {
			char *str = val_getStr(v);
			bool res = *str != '\0';
			efree(str);
			return res;
		}
	}
	/* never reached */
	assert(false);
	return false;
}

sValue *val_cmp(sValue *v1,sValue *v2,u8 op) {
	char *s1 = NULL,*s2 = NULL;
	u8 t1 = v1->type;
	u8 t2 = v2->type;
	tIntType diff;
	bool res = false;
	assert(t1 != TYPE_FUNC && t2 != TYPE_FUNC);
	/* first compute the difference; compare strings if both are strings
	 * compare integers otherwise and convert the values to integers if necessary */
	if(t1 == TYPE_STR && t2 == TYPE_STR) {
		s1 = val_getStr(v1);
		s2 = val_getStr(v2);
		diff = strcmp(s1,s2);
	}
	else if(t1 == TYPE_INT && t2 == TYPE_INT)
		diff = val_getInt(v1) - val_getInt(v2);
	else if(t1 == TYPE_STR) {
		s1 = val_getStr(v1);
		diff = atoi(s1) - val_getInt(v2);
	}
	else {
		s2 = val_getStr(v2);
		diff = val_getInt(v1) - atoi(s2);
	}
	efree(s1);
	efree(s2);
	/* now return the result depending on the difference and operation */
	switch(op) {
		case CMP_OP_EQ:
			res = diff == 0;
			break;
		case CMP_OP_NEQ:
			res = diff != 0;
			break;
		case CMP_OP_LT:
			res = diff < 0;
			break;
		case CMP_OP_GT:
			res = diff > 0;
			break;
		case CMP_OP_LEQ:
			res = diff <= 0;
			break;
		case CMP_OP_GEQ:
			res = diff >= 0;
			break;
	}
	return val_createInt(res);
}

void val_set(sValue *var,sValue *val) {
	assert(val->type != TYPE_FUNC);
	efree(var->v.strval);
	switch(val->type) {
		case TYPE_INT:
			var->v.intval = val_getInt(val);
			break;
		case TYPE_STR:
			var->v.strval = val_getStr(val);
			break;
	}
}

void *val_getFunc(sValue *v) {
	switch(v->type) {
		case TYPE_FUNC:
			return v->v.func;
	}
	assert(false);
	return NULL;
}

tIntType val_getInt(sValue *v) {
	switch(v->type) {
		case TYPE_INT:
			return v->v.intval;
		case TYPE_FUNC:
			return 0;
		case TYPE_STR:
			return atoi(v->v.strval);
	}
	/* never reached */
	assert(false);
	return 0;
}

char *val_getStr(sValue *v) {
	switch(v->type) {
		case TYPE_INT: {
			char *s = (char*)emalloc(12);
			itoa(s,12,v->v.intval);
			return s;
		}
		case TYPE_FUNC:
			return estrdup("<FUNC>");
		case TYPE_STR:
			return estrdup(v->v.strval);
	}
	/* never reached */
	assert(false);
	return NULL;
}
