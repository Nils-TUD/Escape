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

#define ACPI_SIG(A,B,C,D)		(A + (B << 8) + (C << 16) + (D << 24))

/* root system descriptor table */
typedef struct {
	uint32_t signature;
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oemId[6];
	char oemTableId[8];
	uint32_t oemRevision;
	char creatorId[4];
	uint32_t creatorRevision;
} A_PACKED sRSDT;

static __inline__ bool acpi_checksumValid(const void *r,size_t len) {
	uint8_t sum = 0;
	uint8_t *p;
	for(p = (uint8_t*)r; p < (uint8_t*)r + len; p++)
		sum += *p;
	return sum == 0;
}
