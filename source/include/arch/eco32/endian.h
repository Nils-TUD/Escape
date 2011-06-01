/**
 * $Id$
 */

#ifndef ECO32_ENDIAN_H_
#define ECO32_ENDIAN_H_

#include <esc/common.h>

uint16_t le16tocpu(uint16_t in);
uint32_t le32tocpu(uint32_t in);
uint16_t cputole16(uint16_t in);
uint32_t cputole32(uint32_t in);

#endif /* ECO32_ENDIAN_H_ */
