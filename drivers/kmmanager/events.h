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

#ifndef EVENTS_H_
#define EVENTS_H_

#include <esc/common.h>
#include <messages.h>

/**
 * Inits the events-system
 */
void events_init(void);

/**
 * Sends the given keymap-data to all that listen to the corresponding keycode/character,
 * modifier and pressed/released combination
 *
 * @param driver the driver-id
 * @param km the keymap-data
 * @return true if we've sent it to somebody
 */
bool events_send(tDrvId driver,sKmData *km);

/**
 * Adds the given listener
 *
 * @param tid the thread-id of the listener
 * @param flags the flags
 * @param key the key (keycode or character, depending on the flags)
 * @param modifier the modifiers
 * @return 0 if successfull
 */
s32 events_add(tTid tid,u8 flags,u8 key,u8 modifier);

/**
 * Removes the given listener
 *
 * @param tid the thread-id of the listener
 * @param flags the flags
 * @param key the key (keycode or character, depending on the flags)
 * @param modifier the modifiers
 */
void events_remove(tTid tid,u8 flags,u8 key,u8 modifier);

#endif /* EVENTS_H_ */
