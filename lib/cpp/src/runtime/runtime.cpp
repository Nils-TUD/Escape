// -*- C++ -*- Common throw conditions.
// Copyright (C) 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2009
// Free Software Foundation
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
//
// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

#include <stdlib.h>
#include <stdio.h>
#include <runtime/typeinfo>
#include <runtime/cxxabi.h>

extern "C" void __cxa_pure_virtual();
extern "C" void _Unwind_Resume();
extern "C" void __gxx_personality_v0();

/* gcc needs the symbol __dso_handle */
void *__dso_handle;

extern "C" void __cxxabiv1::__cxa_bad_cast() {
	// TODO use exceptions
	fprintf(stderr,"Bad cast!\n");
	exit(1);
}

extern "C" void __cxxabiv1::__cxa_bad_typeid() {
	// TODO use exceptions
	fprintf(stderr,"Bad typeid!\n");
	exit(1);
}

extern "C" void __cxa_pure_virtual() {
	fprintf(stderr,"Called pure virtual method!\n");
	exit(1);
}

extern "C" void _Unwind_Resume() {
	// do nothing
}

extern "C" void __gxx_personality_v0() {
    // do nothing
}
