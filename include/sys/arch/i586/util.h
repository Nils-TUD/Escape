/**
 * $Id$
 */

#ifndef I586_UTIL_H_
#define I586_UTIL_H_

#include <esc/common.h>

/**
 * Outputs the <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void util_outByte(uint16_t port,uint8_t val);

/**
 * Outputs the <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void util_outWord(uint16_t port,uint16_t val);

/**
 * Outputs the <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
extern void util_outDWord(uint16_t port,uint32_t val);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern uint8_t util_inByte(uint16_t port);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern uint16_t util_inWord(uint16_t port);

/**
 * Reads the value from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
extern uint32_t util_inDWord(uint16_t port);

/**
 * @return the address of the stack-frame-start
 */
extern uintptr_t getStackFrameStart(void);

/**
 * Starts the timer
 */
void util_startTimer(void);

/**
 * Stops the timer and displays "<prefix>: <instructions>"
 *
 * @param prefix the prefix to display
 */
void util_stopTimer(const char *prefix,...);

/**
 * Builds the stacktrace with given vars
 */
sFuncCall *util_getStackTrace(uint32_t *ebp,uintptr_t rstart,uintptr_t mstart,uintptr_t mend);

#endif /* I586_UTIL_H_ */
