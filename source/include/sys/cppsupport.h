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

#include <sys/common.h>
#include <sys/mem/cache.h>

class CacheAllocatable {
public:
	static void *operator new(size_t size) throw() {
		return Cache::alloc(size);
	}
	static void operator delete(void *ptr) throw() {
		Cache::free(ptr);
	}
};

#define CREATE(className,...)	({							\
	bool __succ = true;										\
	className *__t = new className(__VA_ARGS__,__succ);		\
	if(!__succ) {											\
		delete __t;											\
		__t = NULL;											\
	}														\
	__t;													\
})

#define CLONE(className,obj,...)	CREATE(className,obj,__VA_ARGS__)

/* unfortunatly, we can't use the template-solution atm, because eco32 uses still gcc 4.4.3 and it
 * will probably be quite hard to move to a newer version because of the backend we would have to
 * port :/ */
#if 0
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
static inline T *create(ARGS && ... args) {
	bool succ = true;
	T *t = new T(args...,succ);
	if(!succ) {
		delete t;
		return NULL;
	}
	return t;
}

template<class T,typename... ARGS>
static inline T *clone(const T &t,ARGS && ... args) {
	return create<T>(t,forward<ARGS...>(args...));
}
#endif
