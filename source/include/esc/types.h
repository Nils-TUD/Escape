/**
 * $Id$
 */

#ifndef TYPES_H_
#define TYPES_H_

#ifndef __cplusplus
typedef enum {false = 0, true = 1} bool;
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef signed long long llong;
typedef unsigned long long ullong;

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef unsigned long long uint64_t;
typedef signed long long int64_t;

#ifdef __mmix__
typedef int64_t intptr_t;
typedef uint64_t uintptr_t;
typedef int64_t ssize_t;
#else
typedef int32_t intptr_t;
typedef uint32_t uintptr_t;
typedef int32_t ssize_t;
#endif

#ifdef __cplusplus
namespace std {
#endif

#ifndef _PTRDIFF_T
#define _PTRDIFF_T
#ifdef __mmix__
typedef int64_t ptrdiff_t;
#else
typedef int32_t ptrdiff_t;
#endif
#endif

#ifndef __SIZE_T__
#define __SIZE_T__
#ifdef __mmix__
typedef uint64_t size_t;
#else
typedef uint32_t size_t;
#endif
#endif

#ifdef __cplusplus
}
#endif

#ifdef __mmix__
typedef int64_t off_t;
#else
typedef int32_t off_t;
#endif
typedef uint16_t mode_t;

/* union to access qwords as 2 dwords */
typedef union {
	struct {
		uint32_t lower;
		uint32_t upper;
	} val32;
	uint64_t val64;
} uLongLong;

typedef uint16_t pid_t;
typedef uint16_t tid_t;
typedef uint16_t uid_t;
typedef uint16_t gid_t;

typedef int32_t inode_t;
typedef uint32_t block_t;
typedef int32_t dev_t;

typedef uint8_t sig_t;

typedef uint32_t msgid_t;

typedef uintptr_t evobj_t;

typedef uint32_t time_t;

#endif /* TYPES_H_ */
