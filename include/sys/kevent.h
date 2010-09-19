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

#ifndef KEVENT_H_
#define KEVENT_H_

#include <sys/common.h>

/* the available kevents */
#define KEV_VFS_REAL		0		/* message received for the kernel => VFSR */
#define KEV_KWAIT_DONE		1		/* a thread that waits in kernel has been waked up */

/* The callback-function */
typedef void (*fKEvCallback)(u32 param);

/**
 * Inits the kevents
 */
void kev_init(void);

/**
 * Adds the callback for the given event. As soon as the function is called it will be removed
 * from kevents
 *
 * @param event the event to wait for
 * @param callback the function to call
 * @return 0 on success
 */
s32 kev_add(u32 event,fKEvCallback callback);

/**
 * Notifies all about the given event
 *
 * @param event the event
 * @param param a parameter to pass to the callback
 * @return the number of notified clients
 */
u32 kev_notify(u32 event,u32 param);

#if DEBUGGING
void kev_dbg_print(void);
#endif

#endif /* KEVENT_H_ */
