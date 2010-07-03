// Copyright (C) 2002, 2004, 2006, 2008, 2009 Free Software Foundation, Inc.
//  
// This file is part of GCC.
//
// GCC is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.

// GCC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

// Written by Mark Mitchell, CodeSourcery LLC, <mark@codesourcery.com>
// Thread support written by Jason Merrill, Red Hat Inc. <jason@redhat.com>

#include "cxxabi.h"

// The IA64/generic ABI uses the first byte of the guard variable.
// The ARM EABI uses the least significant bit.

namespace __gnu_cxx {
	// 6.7[stmt.dcl]/4: If control re-enters the declaration (recursively)
	// while the object is being initialized, the behavior is undefined.

	// Since we already have a library function to handle locking, we might
	// as well check for this situation and throw an exception.
	// We use the second byte of the guard variable to remember that we're
	// in the middle of an initialization.
	class recursive_init_error: public std::exception {
	public:
		recursive_init_error() throw () {
		}
		virtual ~recursive_init_error() throw ();
	};

	recursive_init_error::~recursive_init_error() throw () {
	}
}

//
// Here are C++ run-time routines for guarded initiailization of static
// variables. There are 4 scenarios under which these routines are called:
//
//   1. Threads not supported (__GTHREADS not defined)
//   2. Threads are supported but not enabled at run-time.
//   3. Threads enabled at run-time but __gthreads_* are not fully POSIX.
//   4. Threads enabled at run-time and __gthreads_* support all POSIX threads
//      primitives we need here.
//
// The old code supported scenarios 1-3 but was broken since it used a global
// mutex for all threads and had the mutex locked during the whole duration of
// initlization of a guarded static variable. The following created a dead-lock
// with the old code.
//
//	Thread 1 acquires the global mutex.
//	Thread 1 starts initializing static variable.
//	Thread 1 creates thread 2 during initialization.
//	Thread 2 attempts to acuqire mutex to initialize another variable.
//	Thread 2 blocks since thread 1 is locking the mutex.
//	Thread 1 waits for result from thread 2 and also blocks. A deadlock.
//
// The new code here can handle this situation and thus is more robust. Howere,
// we need to use the POSIX thread conditional variable, which is not supported
// in all platforms, notably older versions of Microsoft Windows. The gthr*.h
// headers define a symbol __GTHREAD_HAS_COND for platforms that support POSIX
// like conditional variables. For platforms that do not support conditional
// variables, we need to fall back to the old code.

// If _GLIBCXX_USE_FUTEX, no global mutex or conditional variable is used,
// only atomic operations are used together with futex syscall.
// Valid values of the first integer in guard are:
// 0				  No thread encountered the guarded init
//				  yet or it has been aborted.
// _GLIBCXX_GUARD_BIT		  The guarded static var has been successfully
//				  initialized.
// _GLIBCXX_GUARD_PENDING_BIT	  The guarded static var is being initialized
//				  and no other thread is waiting for its
//				  initialization.
// (_GLIBCXX_GUARD_PENDING_BIT    The guarded static var is being initialized
//  | _GLIBCXX_GUARD_WAITING_BIT) and some other threads are waiting until
//				  it is initialized.

namespace __cxxabiv1 {
	// TODO just a dummy-implementation

	extern "C" int __cxa_guard_acquire (__guard *g) {
		UNUSED(g);
		return 0;
	}

	extern "C" void __cxa_guard_abort (__guard *g) {
		UNUSED(g);
	}

	extern "C" void __cxa_guard_release (__guard *g) {
		UNUSED(g);
	}
}
