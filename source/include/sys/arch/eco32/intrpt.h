/**
 * $Id$
 */

#ifndef ECO32_INTRPT_H_
#define ECO32_INTRPT_H_

#include <esc/common.h>

#define REG_COUNT 32

/* the saved registers */
typedef struct {
	uint32_t r[REG_COUNT];
	uint32_t psw;
	uint32_t irqNo;
} A_PACKED sIntrptStackFrame;

#endif /* ECO32_INTRPT_H_ */
