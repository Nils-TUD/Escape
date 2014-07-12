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

#pragma once

#include <sys/common.h>
#include <string.h>
#include <assert.h>
#include "vector.h"
#include "../lang.h"

#define VAL_TYPE_INT	0
#define VAL_TYPE_STR	1
#define VAL_TYPE_FUNC	2
#define VAL_TYPE_ARRAY	3

/* a value */
typedef struct {
	uchar type;
	union {
		tIntType intval;
		char *strval;
		void *func;
		sVector *vec;
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
 * Creates an array with given elements
 *
 * @param vals the elements (may be NULL)
 * @return the instance
 */
sValue *val_createArray(sVector *vals);

/**
 * Clones the given value
 *
 * @param v the value
 * @return the clone
 */
sValue *val_clone(const sValue *v);

/**
 * Destroys the given value
 *
 * @param v the value
 */
void val_destroy(sValue *v);

/**
 * Determines the length of the given value
 *
 * @param v the value
 * @return the length
 */
tIntType val_len(const sValue *v);

/**
 * Extracts a sub-part of the given value
 *
 * @param v the value
 * @param start the start-position
 * @param count the number of elements to extract
 * @return the sub-value
 */
sValue *val_sub(const sValue *v,sValue *start,sValue *count);

/**
 * Converts the given value to a space-separated string
 *
 * @param v the value
 * @return the space-separated string-value
 */
sValue *val_tos(const sValue *v);

/**
 * Converts the given value to an array (splits the string by whitespace)
 *
 * @param v the value
 * @return the array-value
 */
sValue *val_toa(const sValue *v);

/**
 * Tests whether the value is true (!= 0 or none-empty string, not allowed for functions)
 *
 * @param v the value
 * @return true or false
 */
bool val_isTrue(const sValue *v);

/**
 * Compares the two values with given compare-operator (not allowed for functions)
 *
 * @param v1 the first value
 * @param v2 the second value
 * @param op the compare-op (CMP_OP_*)
 * @return the compare-result
 */
sValue *val_cmp(const sValue *v1,const sValue *v2,uchar op);

/**
 * Returns the value at given index, if possible.
 *
 * @param array the array
 * @param index the index
 * @return the value at given index or NULL if not possible; this will be no clone!
 */
sValue *val_index(sValue *array,const sValue *index);

/**
 * Appends the given value at given index
 *
 * @param array the array
 * @param val the value to append
 */
void val_append(sValue *array,const sValue *val);

/**
 * Sets the index of the array to given value. will resize the array, if necessary
 *
 * @param array the array
 * @param index the index
 * @param val the value to append
 */
void val_setIndex(sValue *array,const sValue *index,const sValue *val);

/**
 * Sets <var> to the value of <val>
 *
 * @param var the value to change
 * @param val the value to set
 */
void val_set(sValue *var,const sValue *val);

/**
 * @param v the value
 * @return the sFunctionStmt-pointer of the given value. Just allowed for functions
 */
void *val_getFunc(const sValue *v);

/**
 * Returns the integer-representation of the given value (will be converted, if necessary)
 *
 * @param v the value
 * @return the integer
 */
tIntType val_getInt(const sValue *v);

/**
 * Returns the string-representation of the given value (will be converted, if necessary)
 *
 * @param v the value
 * @return the string; you have to free the string!
 */
char *val_getStr(const sValue *v);

/**
 * Returns the array-representation of the given value (will be converted, if necessary)
 * TODO that does not work!!
 *
 * @param v the value
 * @return the vector with the elements; you have to free it!
 */
sVector *val_getArray(const sValue *v);
