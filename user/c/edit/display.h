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

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <esc/common.h>
#include <sllist.h>

#define HOR_MOVE_HOME	0
#define HOR_MOVE_END	1
#define HOR_MOVE_LEFT	2
#define HOR_MOVE_RIGHT	3

void displ_init(sSLList *lineList);

void displ_finish(void);

void displ_getCurPos(s32 *col,s32 *row);

void displ_mvCurHor(u8 type);

void displ_mvCurVertPage(bool up);

void displ_mvCurVert(s32 lineCount);

void displ_markDirty(u32 start,u32 count);

void displ_update(void);

#endif /* DISPLAY_H_ */
