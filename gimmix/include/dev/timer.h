/**
 * $Id: timer.h 162 2011-04-09 18:53:16Z nasmussen $
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "common.h"
#include "core/bus.h"

typedef void (*fTimerCallback)(int param);

/**
 * Initializes the timer-device
 */
void timer_init(void);

/**
 * Performs a tick. Might call event-handlers registered via timer_start().
 */
void timer_tick(void);

/**
 * Registers the given callback to be called after <msec> ticks.
 *
 * @param msec the number of ticks to wait
 * @param device the calling device
 * @param callback the function to call
 * @param param the parameter for the function
 */
void timer_start(octa msec,const sDevice *device,fTimerCallback callback,int param);

#endif /* TIMER_H_ */
