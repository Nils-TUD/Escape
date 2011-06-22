/**
 * $Id$
 */

#ifndef TESTUTILS_H_
#define TESTUTILS_H_

#include <sys/common.h>

void checkMemoryBefore(bool checkMappedPages);
void checkMemoryAfter(bool checkMappedPages);

#endif /* TESTUTILS_H_ */
