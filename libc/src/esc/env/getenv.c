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
#include <esc/proc.h>
#include <string.h>
#include <messages.h>
#include "envintern.h"

bool getEnv(char *value,u32 valSize,const char *name) {
	sMsg msg;
	if(initEnv() < 0)
		return false;
	if(strlen(name) >= sizeof(msg.str.s1))
		return false;

	msg.str.arg1 = getpid();
	strcpy(msg.str.s1,name);
	return doGetEnv(value,&msg,valSize,MSG_ENV_GET,sizeof(msg.str));
}
