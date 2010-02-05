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

#ifndef ENVINTERN_H_
#define ENVINTERN_H_

#include <esc/common.h>
#include <messages.h>

/**
 * Opens the env-service
 * @return >= 0 if successfull
 */
s32 initEnv(void);

/**
 * Sends the given get-message to the env-service and returns the reply-data
 *
 * @param buf the buffer to write to
 * @param msg the message
 * @param bufSize the size of the buffer
 * @param cmd the command
 * @param size the size of the msg
 * @return the received data
 */
bool doGetEnv(char *buf,sMsg *msg,u32 bufSize,u32 cmd,u32 size);

/* the fd for the env-service */
extern tFD envFd;

#endif /* ENVINTERN_H_ */
