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

#include <esc/common.h>
#include <esc/syscalls.h>
#include <esc/arch/i586/ports.h>

int reqports(uint16_t start,size_t count) {
	return syscall2(SYSCALL_REQIOPORTS,start,count);
}

int reqport(uint16_t port) {
	return reqports(port,1);
}

int relports(uint16_t start,size_t count) {
	return syscall2(SYSCALL_RELIOPORTS,start,count);
}

int relport(uint16_t port) {
	return relports(port,1);
}

void inwords(uint16_t port,void *addr,size_t count) {
	__asm__ volatile (
		"rep; insw"
		: : "D"(addr), "c"(count), "d"(port)
	);
}

void outwords(uint16_t port,const void *addr,size_t count) {
	__asm__ volatile (
		"rep; outsw"
		: : "S"(addr), "c"(count), "d"(port)
	);
}
