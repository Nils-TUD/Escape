/**
 * $Id$
 */

#ifndef CONV_H_
#define CONV_H_

#include <esc/common.h>

#ifdef __i386__
#define le16tocpu(x)	(x)
#define le32tocpu(x)	(x)
#define cputole16(x)	(x)
#define cputole32(x)	(x)
#endif

#ifdef __eco32__
uint16_t le16tocpu(uint16_t in);
uint32_t le32tocpu(uint32_t in);
uint16_t cputole16(uint16_t in);
uint32_t cputole32(uint32_t in);
#endif

#endif /* CONV_H_ */
