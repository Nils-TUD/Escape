/**
 * $Id$
 */

#ifndef ERROR_H_
#define ERROR_H_

#include <esc/common.h>

/**
 * The last error-code
 */
extern int errno;

/**
 * Displays an error-message according to given format and arguments and appends ': <errmsg>' if
 * errno is < 0. After that exit(EXIT_FAILURE) is called.
 *
 * @param fmt the error-message-format
 */
void error(const char *fmt,...) A_NORETURN;

#endif /* ERROR_H_ */
