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
#include <esc/util/string.h>
#include <stdlib.h>
#include <esc/width.h>
#include "value.h"
#include "../mem.h"
#include "../ast/cmpexpr.h"
#include "../ast/functionstmt.h"

static sVector *val_cloneArray(const sValue *v);
static void val_destroyValue(sValue *v);
static void val_validateRange(tIntType vallen,tIntType *start,tIntType *count);
static bool val_doCmp(const sValue *v1,const sValue *v2,u8 op);
static void val_ensureArray(sValue *array);
static char *val_arr2Str(const sValue *v,bool brackets);

sValue *val_createInt(tIntType i) {
	sValue *res = (sValue*)emalloc(sizeof(sValue));
	res->type = VAL_TYPE_INT;
	res->v.intval = i;
	return res;
}

sValue *val_createStr(const char *s) {
	sValue *res = (sValue*)emalloc(sizeof(sValue));
	res->type = VAL_TYPE_STR;
	res->v.strval = estrdup(s);
	return res;
}

sValue *val_createFunc(void *func) {
	sValue *res = (sValue*)emalloc(sizeof(sValue));
	res->type = VAL_TYPE_FUNC;
	res->v.func = func;
	return res;
}

sValue *val_createArray(sVector *vals) {
	sValue *res = (sValue*)emalloc(sizeof(sValue));
	res->type = VAL_TYPE_ARRAY;
	/* TODO clone? */
	res->v.vec = vals ? vals : vec_create(sizeof(sValue*));
	return res;
}

sValue *val_clone(const sValue *v) {
	sValue *res = (sValue*)emalloc(sizeof(sValue));
	res->type = v->type;
	switch(v->type) {
		case VAL_TYPE_INT:
			res->v.intval = v->v.intval;
			break;
		case VAL_TYPE_STR:
			res->v.strval = estrdup(v->v.strval);
			break;
		case VAL_TYPE_FUNC:
			res->v.func = v->v.func;
			break;
		case VAL_TYPE_ARRAY:
			res->v.vec = val_cloneArray(v);
			break;
	}
	return res;
}

static sVector *val_cloneArray(const sValue *v) {
	/* make a clone of the vector and elements */
	if(v->type == VAL_TYPE_ARRAY) {
		u32 i;
		sVector *clone = vec_copy(v->v.vec);
		for(i = 0; i < clone->count; i++) {
			sValue *cpy = val_clone((sValue*)vec_get(clone,i));
			vec_set(clone,i,&cpy);
		}
		return clone;
	}
	return vec_create(sizeof(sValue*));
}

void val_destroy(sValue *v) {
	if(v)
		val_destroyValue(v);
	efree(v);
}

static void val_destroyValue(sValue *v) {
	switch(v->type) {
		case VAL_TYPE_FUNC:
			ast_killFunctionStmt((sFunctionStmt*)v->v.func);
			break;
		case VAL_TYPE_STR:
			efree(v->v.strval);
			break;
		case VAL_TYPE_ARRAY: {
			sValue *el;
			vforeach(v->v.vec,el)
				val_destroy(el);
			vec_destroy(v->v.vec,false);
		}
		break;
	}
}

tIntType val_len(const sValue *v) {
	switch(v->type) {
		case VAL_TYPE_INT:
			return getnwidth(v->v.intval);
		case VAL_TYPE_STR:
			return strlen(v->v.strval);
		case VAL_TYPE_ARRAY:
			return v->v.vec->count;
	}
	/* never reached */
	assert(false);
	return 0;
}

sValue *val_sub(const sValue *v,sValue *start,sValue *count) {
	assert(v->type != VAL_TYPE_FUNC);
	tIntType istart = start ? val_getInt(start) : 0;
	tIntType icount = count ? val_getInt(count) : 0;
	switch(v->type) {
		case VAL_TYPE_INT:
		case VAL_TYPE_STR: {
			sValue *res;
			char *str = val_getStr(v);
			tIntType len = strlen(str);
			val_validateRange(len,&istart,&icount);
			str[istart + icount] = '\0';
			res = val_createStr(str + istart);
			efree(str);
			return res;
		}
		case VAL_TYPE_ARRAY: {
			tIntType i;
			sValue *arr = val_createArray(NULL);
			val_validateRange(v->v.vec->count,&istart,&icount);
			for(i = istart; i < istart + icount; i++)
				val_append(arr,vec_get(v->v.vec,i));
			return arr;
		}
	}
	/* never reached */
	assert(false);
	return NULL;
}

static void val_validateRange(tIntType vallen,tIntType *start,tIntType *count) {
	if(*start >= vallen) {
		*start = vallen;
		*count = 0;
		return;
	}
	if(*start < 0)
		*start = 0;
	if(*count == 0)
		*count = vallen - *start;
	else if(*start + *count < *start)
		*count = 0;
	else if(*start + *count > vallen)
		*count = vallen - *start;
}

sValue *val_tos(const sValue *v) {
	sValue *res = NULL;
	assert(v->type != VAL_TYPE_FUNC);
	switch(v->type) {
		case VAL_TYPE_INT:
		case VAL_TYPE_STR: {
			char *str = val_getStr(v);
			res = val_createStr(str);
			efree(str);
		}
		break;
		case VAL_TYPE_ARRAY:
			res = val_createStr(val_arr2Str(v,false));
			break;
	}
	return res;
}

sValue *val_toa(const sValue *v) {
	sValue *res = NULL;
	assert(v->type != VAL_TYPE_FUNC);
	switch(v->type) {
		case VAL_TYPE_INT:
		case VAL_TYPE_STR: {
			res = val_createArray(NULL);
			char *str = val_getStr(v);
			char *tok = strtok(str," \n\t\r");
			while(tok != NULL) {
				sValue *tokVal = val_createStr(tok);
				val_append(res,tokVal);
				val_destroy(tokVal);
				tok = strtok(NULL," \n\t\r");
			}
			efree(str);
		}
		break;
		case VAL_TYPE_ARRAY:
			res = val_clone(v);
			break;
	}
	return res;
}

bool val_isTrue(const sValue *v) {
	switch(v->type) {
		case VAL_TYPE_INT:
			return val_getInt(v) != 0;
		case VAL_TYPE_STR:
			return *(v->v.strval) != '\0';
		case VAL_TYPE_ARRAY:
			return v->v.vec->count > 0;
	}
	/* never reached */
	assert(false);
	return false;
}

sValue *val_cmp(const sValue *v1,const sValue *v2,u8 op) {
	bool res = val_doCmp(v1,v2,op);
	return val_createInt(res);
}

static bool val_doCmp(const sValue *v1,const sValue *v2,u8 op) {
	u8 t1 = v1->type;
	u8 t2 = v2->type;
	tIntType diff;
	assert(t1 != VAL_TYPE_FUNC && t2 != VAL_TYPE_FUNC);
	/* first compute the difference; compare strings if both are strings
	 * compare integers otherwise and convert the values to integers if necessary */
	if(t1 == VAL_TYPE_STR && t2 == VAL_TYPE_STR)
		diff = strcmp(v1->v.strval,v2->v.strval);
	else if(t1 == VAL_TYPE_INT && t2 == VAL_TYPE_INT)
		diff = val_getInt(v1) - val_getInt(v2);
	else if(t1 == VAL_TYPE_ARRAY && t2 == VAL_TYPE_ARRAY) {
		u32 v1c = v1->v.vec->count;
		u32 v2c = v2->v.vec->count;
		/* arrays are equal if all elements are equal.
		 * for all other operators: its true if a compare of one element-pair is true and false
		 * otherwise. So e.g. ['a','b','c'] < ['b','b','c']
		 * If the sizes are different, we can use the 'diff-method' */
		if(v1c != v2c)
			diff = v1c - v2c;
		else {
			u32 i,c = 0;
			for(i = 0; i < v1c; i++) {
				bool res = val_doCmp(vec_get(v1->v.vec,i),vec_get(v2->v.vec,i),op);
				if(res && op != CMP_OP_EQ)
					return res;
				if(res && op == CMP_OP_EQ)
					c++;
			}
			if(op == CMP_OP_EQ && c == v1c)
				return true;
			return false;
		}
	}
	else if(t1 == VAL_TYPE_STR)
		diff = atoi(v1->v.strval) - val_getInt(v2);
	else
		diff = val_getInt(v1) - atoi(v2->v.strval);
	/* now return the result depending on the difference and operation */
	switch(op) {
		case CMP_OP_EQ:
			return diff == 0;
		case CMP_OP_NEQ:
			return diff != 0;
		case CMP_OP_LT:
			return diff < 0;
		case CMP_OP_GT:
			return diff > 0;
		case CMP_OP_LEQ:
			return diff <= 0;
		case CMP_OP_GEQ:
			return diff >= 0;
	}
	assert(false);
	return 0;
}

sValue *val_index(sValue *array,const sValue *index) {
	tIntType i;
	if(array->type != VAL_TYPE_ARRAY)
		return NULL;
	i = val_getInt(index);
	if(i < 0 || i >= (s32)array->v.vec->count)
		return NULL;
	return vec_get(array->v.vec,i);
}

void val_append(sValue *array,const sValue *val) {
	val_ensureArray(array);
	val = val_clone(val);
	vec_add(array->v.vec,&val);
}

void val_setIndex(sValue *array,const sValue *index,const sValue *val) {
	tIntType i = val_getInt(index);
	val_ensureArray(array);
	while(i > (s32)array->v.vec->count) {
		sValue *zero = val_createInt(0);
		vec_add(array->v.vec,&zero);
	}
	val = val_clone(val);
	vec_set(array->v.vec,i,&val);
}

static void val_ensureArray(sValue *array) {
	if(array->type != VAL_TYPE_ARRAY) {
		val_destroyValue(array);
		array->type = VAL_TYPE_ARRAY;
		array->v.vec = vec_create(sizeof(sValue*));
	}
}

void val_set(sValue *var,const sValue *val) {
	assert(val->type != VAL_TYPE_FUNC);
	val_destroyValue(var);
	switch(val->type) {
		case VAL_TYPE_INT:
			var->v.intval = val_getInt(val);
			break;
		case VAL_TYPE_STR:
			var->v.strval = val_getStr(val);
			break;
		case VAL_TYPE_ARRAY:
			var->v.vec = val_cloneArray(val);
			break;
	}
}

void *val_getFunc(const sValue *v) {
	switch(v->type) {
		case VAL_TYPE_FUNC:
			return v->v.func;
	}
	assert(false);
	return NULL;
}

tIntType val_getInt(const sValue *v) {
	switch(v->type) {
		case VAL_TYPE_INT:
			return v->v.intval;
		case VAL_TYPE_ARRAY:
		case VAL_TYPE_FUNC:
			return 0;
		case VAL_TYPE_STR:
			return atoi(v->v.strval);
	}
	/* never reached */
	assert(false);
	return 0;
}

char *val_getStr(const sValue *v) {
	switch(v->type) {
		case VAL_TYPE_INT: {
			char *s = (char*)emalloc(12);
			itoa(s,12,v->v.intval);
			return s;
		}
		case VAL_TYPE_ARRAY:
			return val_arr2Str(v,true);
		case VAL_TYPE_FUNC:
			return estrdup("<FUNC>");
		case VAL_TYPE_STR:
			return estrdup(v->v.strval);
	}
	/* never reached */
	assert(false);
	return NULL;
}

sVector *val_getArray(const sValue *v) {
	switch(v->type) {
		case VAL_TYPE_INT:
		case VAL_TYPE_STR:
			return vec_create(sizeof(sValue*));

		case VAL_TYPE_ARRAY:
			return val_cloneArray(v);
	}
	/* never reached */
	assert(false);
	return NULL;
}

static char *val_arr2Str(const sValue *v,bool brackets) {
	u32 i,len = v->v.vec->count;
	sString *str = str_create();
	if(brackets)
		str_appendc(str,'[');
	for(i = 0; i < len; i++) {
		sValue *el = vec_get(v->v.vec,i);
		char *elstr = val_getStr(el);
		str_append(str,elstr);
		efree(elstr);
		if(i < len - 1)
			str_append(str,brackets ? ", " : " ");
	}
	if(brackets)
		str_appendc(str,']');
	/* TODO maybe we should provide a method for extracting the string and freeing sString? */
	efree(str);
	return str->str;
}
