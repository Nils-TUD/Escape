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

typedef s8* string;
typedef const s8* cstring;
typedef u32 size_t;

/* TODO use <stddef.h>? */
#define NULL (void*)0
typedef enum {false = 0, true = 1} bool;

#define K 1024
#define M 1024 * K
#define G 1024 * M

#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) > (b) ? (b) : (a))

/* debugging */
#define DBG_PROC_CLONE(s) s
#define DBG_KMALLOC(s)

#endif /*COMMON_H_*/
