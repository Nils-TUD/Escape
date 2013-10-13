/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <vbe/vbe.h>

typedef struct {
	uint refs;
	uint8_t *frmbuf;
	uint8_t *whOnBlCache;
	uint8_t *content;
	sScreenMode *mode;
	uint8_t cols;
	uint8_t rows;
	uint8_t lastCol;
	uint8_t lastRow;
} sVESAScreen;

sVESAScreen *vesascr_request(sScreenMode *minfo);
void vesascr_reset(sVESAScreen *scr,int type);
void vesascr_release(sVESAScreen *scr);
