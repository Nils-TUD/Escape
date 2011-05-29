/**
 * $Id$
 */

#include <esc/common.h>
#include <sys/task/timer.h>

#define TIMER_BASE			0xF0000000

#define TIMER_CTRL			0		/* timer control register */
#define TIMER_DIVISOR		1		/* timer divisor register */

#define TIMER_EXP			0x01	/* timer has expired */
#define TIMER_IEN			0x02	/* enable timer interrupt */

void timer_arch_init(void) {
	uint *regs = (uint*)TIMER_BASE;
	/* set frequency */
	regs[TIMER_DIVISOR] = TIMER_FREQUENCY;
	/* enable timer */
	regs[TIMER_CTRL] = TIMER_IEN;
}

void timer_ackIntrpt(void) {
	uint *regs = (uint*)TIMER_BASE;
	/* remove expired-flag */
	regs[TIMER_CTRL] = TIMER_IEN;
}
