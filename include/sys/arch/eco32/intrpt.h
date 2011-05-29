/**
 * $Id$
 */

#ifndef ECO32_INTRPT_H_
#define ECO32_INTRPT_H_

#include <esc/common.h>

#define REG_COUNT 32

/* the saved registers */
typedef struct {
	ulong r[REG_COUNT];
	ulong psw;
} sIntrptRegs;

void irq_handler(int irqNo,sIntrptRegs *regs);

#endif /* ECO32_INTRPT_H_ */
