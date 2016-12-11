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

#include <mem/useraccess.h>
#include <task/thread.h>
#include <common.h>
#include <errno.h>

int UserAccess::copy(char *dst,const char *src,size_t size,size_t offset) {
	Thread *t = Thread::getRunning();
	while(size > 0) {
		copyByte(dst,src);
		if(t->isFaulted())
			return -EFAULT;

		size_t amount = esc::Util::min(size,PAGE_SIZE - offset);
		memcpy(dst,src,amount);
		size -= amount;
		dst += amount;
		src += amount;
		offset = 0;
	}
	return 0;
}

int UserAccess::read(void *dst,USER const void *src,size_t size) {
	const char *bsrc = reinterpret_cast<const char*>(src);
	char *bdst = reinterpret_cast<char*>(dst);
	size_t offset = reinterpret_cast<uintptr_t>(bsrc) & PAGE_SIZE;
	return copy(bdst,bsrc,size,offset);
}

int UserAccess::write(USER void *dst,const void *src,size_t size) {
	const char *bsrc = reinterpret_cast<const char*>(src);
	char *bdst = reinterpret_cast<char*>(dst);
	size_t offset = reinterpret_cast<uintptr_t>(bdst) & PAGE_SIZE;
	return copy(bdst,bsrc,size,offset);
}

ssize_t UserAccess::strlen(USER const char *str,size_t max) {
	Thread *t = Thread::getRunning();
	char c;
	const char *begin = str;
	while(max == 0 || (max-- > 0)) {
		copyByte(&c,str);
		if(t->isFaulted())
			return -EFAULT;
		if(c == '\0')
			return str - begin;
		str++;
	}
	return -EFAULT;
}

int UserAccess::strnzcpy(USER char *dst,USER const char *str,size_t size) {
	Thread *t = Thread::getRunning();
	while(size-- > 1) {
		copyByte(dst,str);
		if(t->isFaulted())
			return -EFAULT;
		if(*str == '\0')
			break;
		dst++;
		str++;
	}
	char c = '\0';
	copyByte(dst,&c);
	if(t->isFaulted())
		return -EFAULT;
	return 0;
}
