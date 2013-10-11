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
#include <esc/io.h>
#include <esc/messages.h>

#define WIDTH				(mode.cols)
#define HEIGHT				(mode.rows)
#define PADDING				1
#define GWIDTH				(WIDTH - PADDING * 2)
#define GHEIGHT				(HEIGHT - PADDING * 2)

extern sScreenMode mode;

void ui_init(uint cols,uint rows);
void ui_destroy(void);
void ui_update(void);
