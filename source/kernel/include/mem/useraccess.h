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

#include <common.h>

class UserAccess {
	UserAccess() = delete;

public:
	static int read(void *dst,USER const void *src,size_t size);
	static int write(USER void *dst,const void *src,size_t size);

	template<typename T>
	static int readVar(T *dst,USER const T *src) {
		return read(dst,src,sizeof(T));
	}
	template<typename T>
	static int writeVar(USER T *dst,T src) {
		return write(dst,&src,sizeof(T));
	}

	static ssize_t strlen(USER const char *str,size_t max = 0);
	static int strnzcpy(USER char *dst,USER const char *str,size_t size);
	static void copyByte(USER char *dst,USER const char *src);

private:
	static int copy(char *dst,const char *src,size_t size,size_t offset);
};

#if defined(__i586__)
#	include <arch/i586/mem/useraccess.h>
#elif defined(__x86_64__)
#	include <arch/x86_64/mem/useraccess.h>
#elif defined(__eco32__)
#	include <arch/eco32/mem/useraccess.h>
#elif defined(__mmix__)
#	include <arch/mmix/mem/useraccess.h>
#endif
