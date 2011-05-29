/**
 * $Id$
 */

#ifndef ECO32_TIMER_H_
#define ECO32_TIMER_H_

#include <esc/common.h>

/**
 * Inits the architecture-dependent part of the timer
 */
void timer_arch_init(void);

/**
 * Acknoleges a timer-interrupt
 */
void timer_ackIntrpt(void);

#endif /* ECO32_TIMER_H_ */
