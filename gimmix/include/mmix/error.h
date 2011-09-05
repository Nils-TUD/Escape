/**
 * $Id: error.h 153 2011-04-06 16:46:17Z nasmussen $
 */

#ifndef ERROR_H_
#define ERROR_H_

#include <stdarg.h>

/**
 * Prints the given message, like printf, and exits with EXIT_FAILURE
 *
 * @param msg the message
 */
void error(const char *msg,...);

/**
 * Prints the given message, like printf, and exits with EXIT_FAILURE
 *
 * @param msg the message
 * @param ap the arguments
 */
void verror(const char *msg,va_list ap);

#endif /* ERROR_H_ */
