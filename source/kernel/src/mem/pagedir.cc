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

#include <sys/common.h>
#include <sys/mem/pagedir.h>
#include <sys/task/thread.h>
#include <sys/util.h>

void PageDir::RangeAllocator::freePage(frameno_t) {
	Util::panic("Not supported");
}

PageDir::UAllocator::UAllocator() : Allocator(), _thread(Thread::getRunning()) {
}

frameno_t PageDir::UAllocator::allocPage() {
	return _thread->getFrame();
}

frameno_t PageDir::KAllocator::allocPT() {
	Util::panic("Trying to allocate a page-table in kernel-area");
	return 0;
}

void PageDir::KAllocator::freePT(frameno_t) {
	Util::panic("Trying to free a page-table in kernel-area");
}
