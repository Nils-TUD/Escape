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
#include <esc/heap.h>
#include "value.h"

sDynVal *val_createInt(s32 i) {
	sDynVal *res = (sDynVal*)malloc(sizeof(sDynVal));
	if(res) {
		res->type = TYPE_INT;
		res->intval = i;
		res->strval = NULL;
		res->varval = NULL;
	}
	return res;
}

sDynVal *val_createStr(const char *s) {
	sDynVal *res = (sDynVal*)malloc(sizeof(sDynVal));
	if(res) {
		res->type = TYPE_STR;
		res->intval = 0;
		res->strval = strdup(s);
		res->varval = NULL;
	}
	return res;
}

sDynVal *val_createVar(const char *s) {
	sDynVal *res = (sDynVal*)malloc(sizeof(sDynVal));
	if(res) {
		res->type = TYPE_VAR;
		res->intval = 0;
		res->strval = strdup(s);
		res->varval = val_createInt(0);
	}
	return res;
}

void val_destroy(sDynVal *v) {
	if(v) {
		switch(v->type) {
			case TYPE_STR:
			case TYPE_VAR:
				free(v->strval);
				val_destroy(v->varval);
				break;
		}
	}
}

u8 val_getType(sDynVal *v) {
	if(v->type == TYPE_VAR)
		return val_getType(v->varval);
	return v->type;
}

bool val_isTrue(sDynVal *v) {
	u8 type = val_getType(v);
	switch(type) {
		case TYPE_INT:
			return val_getInt(v) != 0;
		case TYPE_STR:
			return strcmp(val_getStr(v),"") != 0;
		default:
			/* never reached */
			vassert(false,"Invalid type: %d",type);
			return false;
	}
}

sDynVal *val_cmp(sDynVal *v1,sDynVal *v2,u8 op) {
	u8 t1 = val_getType(v1);
	u8 t2 = val_getType(v2);
	/* first compute the difference; compare strings if both are strings
	 * compare integers otherwise and convert the values to integers if necessary */
	s32 diff;
	if(t1 == TYPE_STR && t2 == TYPE_STR)
		diff = strcmp(val_getStr(v1),val_getStr(v2));
	else if(t1 == TYPE_INT && t2 == TYPE_INT)
		diff = val_getInt(v1) - val_getInt(v2);
	else if(t1 == TYPE_STR)
		diff = atoi(val_getStr(v1)) - val_getInt(v2);
	else
		diff = val_getInt(v1) - atoi(val_getStr(v2));
	/* now return the result depending on the difference and operation */
	bool res;
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
		default:
			/* never reached */
			vassert(false,"Invalid op: %d",op);
			res = 0;
			break;
	}
	return val_createInt(res);
}

void val_set(sDynVal *var,sDynVal *val) {
	var->intval = 0;
	free(var->strval);
	switch(val->type) {
		case TYPE_INT:
			var->intval = val_getInt(val);
			break;
		case TYPE_STR:
			var->strval = val_getStr(val);
			break;
		case TYPE_VAR:
			val_set(var,val->varval);
			break;
		default:
			/* never reached */
			vassert(false,"Invalid type: %d",val->type);
			break;
	}
}

s32 val_getInt(sDynVal *v) {
	switch(v->type) {
		case TYPE_INT:
			return v->intval;
		case TYPE_STR:
			return atoi(v->strval);
		case TYPE_VAR:
			return val_getInt(v->varval);
		default:
			/* never reached */
			vassert(false,"Invalid type: %d",v->type);
			return 0;
	}
}

char *val_getStr(sDynVal *v) {
	switch(v->type) {
		case TYPE_INT: {
			char *s = (char*)malloc(12);
			if(s)
				itoa(s,v->intval);
			return s;
		}
		case TYPE_STR:
			return strdup(v->strval);
		case TYPE_VAR:
			return val_getStr(v->varval);
		default:
			/* never reached */
			vassert(false,"Invalid type: %d",v->type);
			return NULL;
	}
}
