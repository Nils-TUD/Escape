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
#include "../lang.h"

#define TYPE_INT	0
#define TYPE_STR	1

typedef struct {
	u8 type;
	tIntType intval;
	char *strval;
} sValue;

sValue *val_createInt(tIntType i);

sValue *val_createStr(const char *s);

sValue *val_clone(sValue *v);

void val_destroy(sValue *v);

bool val_isTrue(sValue *v);

sValue *val_cmp(sValue *v1,sValue *v2,u8 op);

void val_set(sValue *var,sValue *val);

tIntType val_getInt(sValue *v);

char *val_getStr(sValue *v);

#endif /* VALUE_H_ */
