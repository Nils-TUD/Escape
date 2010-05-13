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

#define VAL_TYPE_INT	0
#define VAL_TYPE_STR	1
#define VAL_TYPE_FUNC	2

/* a value */
typedef struct {
	u8 type;
	union {
		tIntType intval;
		char *strval;
		void *func;
	} v;
} sValue;

/**
 * Creates an integer
 *
 * @param i the value
 * @return the instance
 */
sValue *val_createInt(tIntType i);

/**
 * Creates a string
 *
 * @param s the string (will be copied)
 * @return the instance
 */
sValue *val_createStr(const char *s);

/**
 * Creates a function
 *
 * @param n the sFunctionStmt-pointer
 * @return the instance
 */
sValue *val_createFunc(void *n);

/**
 * Clones the given value
 *
 * @param v the value
 * @return the clone
 */
sValue *val_clone(sValue *v);

/**
 * Destroys the given value
 *
 * @param v the value
 */
void val_destroy(sValue *v);

/**
 * Tests wether the value is true (!= 0 or none-empty string, not allowed for functions)
 *
 * @param v the value
 * @return true or false
 */
bool val_isTrue(sValue *v);

/**
 * Compares the two values with given compare-operator (not allowed for functions)
 *
 * @param v1 the first value
 * @param v2 the second value
 * @param op the compare-op (CMP_OP_*)
 * @return the compare-result
 */
sValue *val_cmp(sValue *v1,sValue *v2,u8 op);

/**
 * Sets <var> to the value of <val>
 *
 * @param var the value to change
 * @param val the value to set
 */
void val_set(sValue *var,sValue *val);

/**
 * @param v the value
 * @return the sFunctionStmt-pointer of the given value. Just allowed for functions
 */
void *val_getFunc(sValue *v);

/**
 * Returns the integer-representation of the given value (will be converted, if necessary)
 *
 * @param v the value
 * @return the integer
 */
tIntType val_getInt(sValue *v);

/**
 * Returns the string-representation of the given value (will be converted, if necessary)
 *
 * @param v the value
 * @return the string; you have to free the string!
 */
char *val_getStr(sValue *v);

#endif /* VALUE_H_ */
