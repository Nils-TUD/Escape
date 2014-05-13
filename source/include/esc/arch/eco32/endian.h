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

#include <esc/common.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define be16tocpu(x)	(x)
#define be32tocpu(x)	(x)
#define cputobe16(x)	(x)
#define cputobe32(x)	(x)

uint16_t le16tocpu(uint16_t in);
uint32_t le32tocpu(uint32_t in);
uint16_t cputole16(uint16_t in);
uint32_t cputole32(uint32_t in);

#if defined(__cplusplus)
}
#endif
