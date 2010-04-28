// This file is part of the uSTL library, an STL implementation.
//
// Copyright (c) 2005-2009 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#ifndef UNEW_H_11D237512B324C9C05A55DAF1BF086F1
#define UNEW_H_11D237512B324C9C05A55DAF1BF086F1

#include "uexception.h"
#include <stdlib.h>

/// Just like malloc, but throws on failure.
void* tmalloc (size_t n) throw (ustl::bad_alloc) __attribute__((malloc));
/// Just like free, but doesn't crash when given a NULL.
inline void nfree (void* p) throw() { if (p) free (p); }

#if WITHOUT_LIBSTDCPP

//
// These are replaceable signatures:
//  - normal single new and delete (no arguments, throw @c bad_alloc on error)
//  - normal array new and delete (same)
//  - @c nothrow single new and delete (take a @c nothrow argument, return
//    @c NULL on error)
//  - @c nothrow array new and delete (same)
//
//  Placement new and delete signatures (take a memory address argument,
//  does nothing) may not be replaced by a user's program.
//
inline void* operator new (size_t n) throw (ustl::bad_alloc)	{ return (tmalloc (n)); }
inline void* operator new[] (size_t n) throw (ustl::bad_alloc)	{ return (tmalloc (n)); }
inline void  operator delete (void* p) throw()			{ nfree (p); }
inline void  operator delete[] (void* p) throw()		{ nfree (p); }

// Default placement versions of operator new.
inline void* operator new (size_t, void* p) throw() { return (p); }
inline void* operator new[] (size_t, void* p) throw() { return (p); }

// Default placement versions of operator delete.
inline void  operator delete  (void*, void*) throw() { }
inline void  operator delete[](void*, void*) throw() { }

#else
#include <new>
#endif	// WITHOUT_LIBSTDCPP

#endif
