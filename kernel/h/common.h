/**
 * @version		$Id: common.h 77 2008-11-22 22:27:35Z nasmussen $
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

/* process id */
typedef u16 tPid;
/* VFS node number */
typedef u32 tVFSNodeNo;
/* file-number (in global file table) */
typedef s32 tFile;
/* file-descriptor */
typedef u16 tFD;
/* inode-number */
typedef u32 tInodeNo;


/* TODO use <stddef.h>? */
#define NULL (void*)0
typedef enum {false = 0, true = 1} bool;

#define K 1024
#define M 1024 * K
#define G 1024 * M

#ifndef DEBUGGING
#define DEBUGGING 1
#endif

#if DEBUGGING
#define ASSERT(cond,errorMsg,...) if(!(cond)) { \
		panic("Assert failed at %s, %s() line %d: " errorMsg,__FILE__,__FUNCTION__,\
				__LINE__,## __VA_ARGS__); \
	}
#else
#define ASSERT(cond,errorMsg,...)
#endif

#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) > (b) ? (b) : (a))

/* for declaring unused parameters */
#define UNUSED(x) (void)(x)

/* error-codes */
#define ERR_FILE_IN_USE				-1
#define ERR_NO_FREE_FD				-2
#define	ERR_MAX_PROC_FDS			-3
#define ERR_VFS_NODE_NOT_FOUND		-4
#define ERR_INVALID_SYSC_ARGS		-5
#define ERR_INVALID_FD				-6
#define ERR_INVALID_FILE			-7
#define ERR_NO_READ_PERM			-8
#define ERR_NO_WRITE_PERM			-9
#define ERR_INV_SERVICE_NAME		-10
#define ERR_NOT_ENOUGH_MEM			-11
#define ERR_SERVICE_EXISTS			-12
#define ERR_PROC_DUP_SERVICE		-13
#define ERR_PROC_DUP_SERVICE_USE	-14
#define ERR_SERVICE_NOT_IN_USE		-15
#define ERR_NOT_OWN_SERVICE			-16
#define ERR_IO_MAP_RANGE_RESERVED	-17
#define ERR_IOMAP_NOT_PRESENT		-18
#define ERR_INTRPT_LISTENER_MSGLEN	-19
#define ERR_INVALID_IRQ_NUMBER		-20
#define ERR_IRQ_LISTENER_MISSING	-21
#define ERR_NO_CLIENT_WAITING		-22

/* debugging */
#define DBG_PGCLONEPD(s)
#define DBG_KMALLOC(s)

#endif /*COMMON_H_*/
