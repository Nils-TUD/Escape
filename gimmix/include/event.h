/**
 * $Id: event.h 202 2011-05-10 10:49:04Z nasmussen $
 */

#ifndef EVENT_H_
#define EVENT_H_

#include "common.h"

#define EV_BEFORE_EXEC		0	// 0 args
#define EV_AFTER_EXEC		1	// 0 args
#define EV_CPU_HALTED		2	// 0 args
#define EV_EXCEPTION		3	// 0 args
#define EV_VAT				4	// 2 args: addr,mode
#define EV_BEFORE_FETCH		5	// 0 args
#define EV_STACKLOAD		6	// 2 args: type,index
#define EV_STACKSTORE		7	// 2 args: type,index
#define EV_DEV_READ			8	// 2 args: addr,data
#define EV_DEV_WRITE		9	// 2 args: addr,data
#define EV_DEV_CALLBACK		10	// 2 args: the device, param
#define EV_CPU_PAUSE		11	// 0 args
#define EV_CACHE_LOOKUP		12	// 2 args: cache,addr
#define EV_CACHE_FILL		13	// 2 args: cache,addr
#define EV_CACHE_FLUSH		14	// 2 args: cache,addr
#define EV_USER_INT			15	// 0 args: fired when the sim gets SIGINT. be carefull: will be
								// fired in the signal-handler!
#define EV_COUNT			16

typedef struct {
	int event;
	octa arg1;
	octa arg2;
} sEvArgs;

/**
 * Signature of event-handlers
 */
typedef void (*fEvent)(const sEvArgs *args);

/**
 * Shuts down the event-module
 */
void ev_shutdown(void);

/**
 * Registers <func> for the event <event>.
 *
 * @param event the event to listen for
 * @param func the function to call
 */
void ev_register(int event,fEvent func);

/**
 * Unregisters <func> from <event>
 *
 * @param event the event
 * @param func the function
 */
void ev_unregister(int event,fEvent func);

/**
 * Fires the given event. That means, all registered handlers are called.
 *
 * @param event the event
 * @param arg1 the first argument
 * @param arg2 the second argument
 */
void ev_fire(int event);
void ev_fire1(int event,octa arg1);
void ev_fire2(int event,octa arg1,octa arg2);

#endif /* EVENT_H_ */
