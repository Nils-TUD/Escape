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

#include <sys/common.h>
#include <sys/mem/cache.h>
#include <sys/util.h>

EXTERN_C int __cxa_atexit(void (*f)(void *), void *p, void *d);
EXTERN_C void __cxa_pure_virtual();

void *__dso_handle;

int __cxa_atexit(void (*)(void *), void *, void *) {
	/* just do nothing here because the kernel will never be destructed */
	return 0;
}

void __cxa_pure_virtual() {
	Util::panic("Pure virtual method called");
}
