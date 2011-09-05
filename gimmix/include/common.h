/**
 * $Id: common.h 217 2011-06-03 18:11:57Z nasmussen $
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint8_t byte;
typedef uint16_t wyde;
typedef uint32_t tetra;
typedef uint64_t octa;
typedef int64_t socta;

typedef union {
	double d;
	octa o;
} uDouble;

#define ARRAY_SIZE(a)		(sizeof((a)) / sizeof((a)[0]))
#define MIN(a,b)			((a) > (b) ? (b) : (a))
#define MAX(a,b)			((a) > (b) ? (a) : (b))

// if a parameter is unused and the warning should be suppressed
#define UNUSED(x)			((void)(x))

#endif /* COMMON_H_ */
