/**
 * $Id: keymap.us.h 202 2009-04-11 16:07:24Z nasmussen $
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

#ifndef KEYMAP_US_H_
#define KEYMAP_US_H_

#include <esc/common.h>
#include "keymap.h"

/**
 * Returns the keymap-entry for the given keycode or NULL if the keycode is invalid
 *
 * @param keycode the keycode
 * @return the keymap-entry
 */
sKeymapEntry *keymap_us_get(u8 keyCode);

#endif /* KEYMAP_US_H_ */
