/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef COMMON_H_
#define COMMON_H_

/* basic type-defs */
typedef char s8;
typedef unsigned char u8;
typedef short s16;
typedef unsigned short u16;
typedef long s32;
typedef unsigned long u32;
typedef long long s64;
typedef unsigned long long u64;

typedef u8 bool;
typedef s8 *string;
typedef u32 size_t;

/* TODO use <stddef.h>? */
#define NULL (void*)0
#define true 1
#define false 0

#define K 1024
#define M 1024 * K
#define G 1024 * M

#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))


/* debugging */
#define DBG_PROC_CLONE(s) s

#endif /*COMMON_H_*/
