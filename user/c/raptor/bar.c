/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <esc/common.h>
#include "display.h"
#include "bar.h"

#define BAR_WIDTH		6

static size_t pos;

void bar_init(void) {
	pos = 0;
}

void bar_getDim(size_t *start,size_t *end) {
	*start = pos;
	*end = pos + BAR_WIDTH - 1;
}

void bar_moveLeft(void) {
	if(pos > 0)
		pos--;
}

void bar_moveRight(void) {
	if(pos + BAR_WIDTH <= GWIDTH)
		pos++;
}
