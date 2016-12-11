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

#include "ui.h"

class Bar {
public:
	static const size_t BAR_WIDTH = 6;

	explicit Bar() : pos(0) {
	}

	void getDim(size_t *start,size_t *end) const {
		*start = pos;
		*end = pos + BAR_WIDTH - 1;
	}

	void moveLeft() {
		if(pos > 0)
			pos--;
	}

	void moveRight(UI &ui) {
		if(pos + BAR_WIDTH <= ui.gameWidth())
			pos++;
	}

private:
	size_t pos;
};
