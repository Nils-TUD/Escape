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

#ifndef VALUE_H_
#define VALUE_H_

#include <esc/common.h>
#include <string.h>
#include <assert.h>

#define TYPE_INT	0
#define TYPE_STR	1
#define TYPE_VAR	2

#define CMP_OP_EQ	0
#define CMP_OP_NEQ	1
#define CMP_OP_LT	2
#define CMP_OP_GT	3
#define CMP_OP_LEQ	4
#define CMP_OP_GEQ	5

typedef struct sDynVal sDynVal;
struct sDynVal {
	u8 type;
	s32 intval;
	char *strval;
	sDynVal *varval;
};

sDynVal *val_createInt(s32 i);

sDynVal *val_createStr(const char *s);

sDynVal *val_createVar(const char *s);

void val_destroy(sDynVal *v);

u8 val_getType(sDynVal *v);

bool val_isTrue(sDynVal *v);

sDynVal *val_cmp(sDynVal *v1,sDynVal *v2,u8 op);

void val_set(sDynVal *var,sDynVal *val);

s32 val_getInt(sDynVal *v);

char *val_getStr(sDynVal *v);

#endif /* VALUE_H_ */
