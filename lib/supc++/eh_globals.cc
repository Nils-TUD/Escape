// -*- C++ -*- Manage the thread-local exception globals.
// Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2009
// Free Software Foundation, Inc.
//
// This file is part of GCC.
//
// GCC is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// GCC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

#include "exception"
#include <stdlib.h>
#include "cxxabi.h"
#include "unwind-cxx.h"

// In a freestanding environment, these functions may not be
// available -- but for now, we assume that they are.
extern "C" void *malloc (std::size_t);
extern "C" void free(void *);

using namespace __cxxabiv1;

namespace {
	abi::__cxa_eh_globals*
	get_global() throw () {
		static __thread abi::__cxa_eh_globals global;
		return &global;
	}
} // anonymous namespace

extern "C" __cxa_eh_globals* __cxxabiv1:: __cxa_get_globals_fast() throw() {
	return get_global();
}

extern "C" __cxa_eh_globals* __cxxabiv1:: __cxa_get_globals() throw() {
	return get_global();
}
