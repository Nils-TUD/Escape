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

#include <mem/cache.h>
#include <common.h>

class CacheAllocatable {
public:
	static void *operator new(size_t size) throw() {
		return Cache::alloc(size);
	}
	static void operator delete(void *ptr) throw() {
		Cache::free(ptr);
	}
};

template<typename T>
struct remove_reference {
	typedef T type;
};
template<typename T>
struct remove_reference<T&> {
	typedef T type;
};
template<typename T>
struct remove_reference<T&&> {
	typedef T type;
};

template<typename T>
static constexpr T&& forward(typename remove_reference<T>::type& a) {
	return static_cast<T&&>(a);
}

template<class T,typename... ARGS>
static inline T *createObj(ARGS && ... args) {
	bool succ = true;
	T *t = new T(args...,succ);
	if(!succ) {
		delete t;
		return NULL;
	}
	return t;
}

template<class T,typename... ARGS>
static inline T *cloneObj(const T &t,ARGS && ... args) {
	return createObj<T>(t,forward<ARGS...>(args...));
}
