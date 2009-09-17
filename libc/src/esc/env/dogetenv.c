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

#include <esc/common.h>
#include <esc/env.h>
#include <esc/io.h>
#include <messages.h>
#include <string.h>
#include "envintern.h"

bool doGetEnv(char *buf,u32 bufSize,u32 cmd,u32 size) {
	tMsgId mid;

	/* send message */
	if(send(envFd,cmd,&msg,size) < 0)
		return false;

	/* wait for reply */
	if(receive(envFd,&mid,&msg,sizeof(msg)) <= 0)
		return false;

	memcpy(buf,msg.str.s1,MIN(bufSize,msg.str.arg1));
	return msg.str.arg1 > 0;
}
