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

#ifndef KB_H_
#define KB_H_

#include <sys/common.h>

#define KEV_PRESS	1
#define KEV_RELEASE	2

#define KE_SHIFT	1
#define KE_CTRL		2
#define KE_ALT		4
#define KE_BREAK	8

typedef struct {
	uint8_t keycode;
	char character;
	uint8_t flags;
} sKeyEvent;

/**
 * Fills the given keyevent. If <wait> is true, it waits until a scancode is present. Otherwise
 * it just checks wether one is present. If no, it gives up.
 *
 * @param ev the event to fill (may be NULL if you just want to wait for a keypress/-release)
 * @param events a mask of events to react on (KEV_*)
 * @param wait wether to wait until a scancode is present
 * @return true if a key could be read. false if not (always true, when <wait> is true)
 */
bool kb_get(sKeyEvent *ev,uint8_t events,bool wait);

#endif /* KB_H_ */
