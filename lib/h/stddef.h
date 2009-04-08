/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef STDDEF_H_
#define STDDEF_H_

#include <types.h>

#define offsetof(type,field)	((size_t)(&((type *)0)->field))

#define NULL					(void*)0

#define K						1024
#define M						(1024 * K)
#define G						(1024 * M)

#define ARRAY_SIZE(array)		(sizeof((array)) / sizeof((array)[0]))

#define MAX(a,b)				((a) > (b) ? (a) : (b))
#define MIN(a,b)				((a) > (b) ? (b) : (a))

/* for declaring unused parameters */
#define UNUSED(x)				(void)(x)

/* process id */
typedef u16 tPid;
/* VFS node number */
typedef u32 tVFSNodeNo;
/* file-descriptor */
typedef s16 tFD;
/* inode-number */
typedef s32 tInodeNo;
/* signal-number */
typedef u8 tSig;
/* service-id */
typedef s32 tServ;

typedef u32 ptrdiff_t;
typedef u32 size_t;

#endif /* STDDEF_H_ */
