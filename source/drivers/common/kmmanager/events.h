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
#include <esc/messages.h>

/**
 * Inits the events-system
 */
void events_init(void);

/**
 * Sends the given keymap-data to all that listen to the corresponding keycode/character,
 * modifier and pressed/released combination
 *
 * @param device the device-id
 * @param km the keymap-data
 * @return true if we've sent it to somebody
 */
bool events_send(int device,sKmData *km);

/**
 * Adds the given listener
 *
 * @param id the client-id of the listener
 * @param flags the flags
 * @param key the key (keycode or character, depending on the flags)
 * @param modifier the modifiers
 * @return 0 if successfull
 */
int events_add(inode_t id,uchar flags,uchar key,uchar modifier);

/**
 * Removes the given listener
 *
 * @param id the client-id of the listener
 * @param flags the flags
 * @param key the key (keycode or character, depending on the flags)
 * @param modifier the modifiers
 */
void events_remove(inode_t id,uchar flags,uchar key,uchar modifier);
