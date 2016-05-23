/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <sys/common.h>

static inline uint16_t le16tocpu(uint16_t val) {
	return val;
}
static inline uint32_t le32tocpu(uint32_t val) {
	return val;
}
static inline uint16_t cputole16(uint16_t val) {
	return val;
}
static inline uint32_t cputole32(uint32_t val) {
	return val;
}

static inline uint16_t be16tocpu(uint16_t val) {
	return ((val & 0xFF) << 8) | (val >> 8);
}
static inline uint32_t be32tocpu(uint32_t val) {
	return ((val & 0xFF) << 24) |
			((val & 0xFF00) << 8) |
			((val & 0xFF0000) >> 8) |
			(val >> 24);
}
static inline uint16_t cputobe16(uint16_t val) {
	return be16tocpu(val);
}
static inline uint32_t cputobe32(uint32_t val) {
	return be32tocpu(val);
}
