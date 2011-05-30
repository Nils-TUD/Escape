/**
 * $Id$
 */

#include <esc/common.h>
#include "../../conv.h"

uint16_t le16tocpu(uint16_t in) {
	return ((in >> 8) & 0xFF) << 0 |
			((in >> 0) & 0xFF) << 8;
}

uint32_t le32tocpu(uint32_t in) {
	return ((in >> 24) & 0xFF) << 0 |
			((in >> 16) & 0xFF) << 8 |
			((in >> 8) & 0xFF) << 16 |
			((in >> 0) & 0xFF) << 24;
}

uint16_t cputole16(uint16_t in) {
	return ((in >> 8) & 0xFF) << 0 |
			((in >> 0) & 0xFF) << 8;
}

uint32_t cputole32(uint32_t in) {
	return ((in >> 24) & 0xFF) << 0 |
			((in >> 16) & 0xFF) << 8 |
			((in >> 8) & 0xFF) << 16 |
			((in >> 0) & 0xFF) << 24;
}
