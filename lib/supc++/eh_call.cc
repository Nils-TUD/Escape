// -*- C++ -*- Helpers for calling unextected and terminate
// Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009
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

#include "exception_defines.h"
#include "unwind-cxx.h"
#include <stdlib.h>

using namespace __cxxabiv1;

#include "unwind-pe.h"

// Helper routine for when the exception handling code needs to call
// terminate.

extern "C" void __cxa_call_terminate(_Unwind_Exception* ue_header) {
	if(ue_header) {
		// terminate is classed as a catch handler.
		__cxa_begin_catch(ue_header);

		// Call the terminate handler that was in effect when we threw this
		// exception.  */
		if(__is_gxx_exception_class(ue_header->exception_class)) {
			__cxa_exception* xh;

			xh = __get_exception_header_from_ue(ue_header);
			__terminate(xh->terminateHandler);
		}
	}
	/* Call the global routine if we don't have anything better.  */
	std::terminate();
}
