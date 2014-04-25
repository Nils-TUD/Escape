/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <sys/common.h>

class UserAccess {
	UserAccess() = delete;

public:
	static int read(void *dst,USER const void *src,size_t size);
	static int write(USER void *dst,const void *src,size_t size);
	static ssize_t strlen(USER const char *str,size_t max = 0);
	static int strnzcpy(char *dst,USER const char *str,size_t size);
	static void copyByte(char *dst,const char *src);

private:
	static int copy(char *dst,const char *src,size_t size,size_t offset);
};

#ifdef __i386__
#include <sys/arch/i586/mem/useraccess.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/mem/useraccess.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/mem/useraccess.h>
#endif
