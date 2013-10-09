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
#include <vterm/vtctrl.h>

/**
 * Inits the given vterm
 *
 * @param id the device-id
 * @param vterm the vterm
 * @param name the device name
 * @param cols the number of desired cols
 * @param rows the number of desired rows
 * @return true if successfull
 */
bool vt_init(int id,sVTerm *vterm,const char *name,uint cols,uint rows);

/**
 * Updates the dirty range
 *
 * @param vt the vterm
 */
void vt_update(sVTerm *vt);
