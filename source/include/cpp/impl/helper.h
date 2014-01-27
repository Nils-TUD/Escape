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

#include <esc/common.h>

namespace std {
	// this is mostly inspired by the stuff from gcc's libstdc++.

	// bool types
	template<bool V>
	struct bool_base {
		static const bool value = V;
	};
	typedef bool_base<true> true_type;
	typedef bool_base<false> false_type;

	// is_array
	template<typename T>
	struct is_array : public false_type {
	};
	template<typename T,size_t Size>
	struct is_array<T[Size]> : public true_type {
	};
	template<typename T>
	struct is_array<T[]> : public true_type {
	};

	// is_function
	template<typename T>
	struct is_function : public false_type {
	};
	template<typename Res,typename... Args>
	struct is_function<Res(Args...)> : public true_type {
	};
	template<typename Res,typename... Args>
	struct is_function<Res(Args...) const> : public true_type {
	};
	template<typename Res,typename... Args>
	struct is_function<Res(Args...) volatile> : public true_type {
	};
	template<typename Res,typename... Args>
	struct is_function<Res(Args...) const volatile> : public true_type {
	};

	// remove_reference
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

	// remove_const
	template<typename T>
	struct remove_const {
		typedef T type;
	};
	template<typename T>
	struct remove_const<const T> {
		typedef T type;
	};
	// remove_volatile
	template<typename T>
	struct remove_volatile {
		typedef T type;
	};
	template<typename T>
	struct remove_volatile<volatile T> {
		typedef T type;
	};
	// remove_cv
	template<typename T>
	struct remove_cv {
		typedef typename remove_volatile<typename remove_const<T>::type>::type type;
	};
	// remove_pointer
	template<typename T,typename>
	struct remove_pointer_base {
		typedef T type;
	};
	template<typename T,typename U>
	struct remove_pointer_base<T,U*> {
		typedef U type;
	};
	template<typename T>
	struct remove_pointer : public remove_pointer_base<T,typename remove_cv<T>::type> {
	};
	// remove_extent
	template<typename T>
	struct remove_extent {
		typedef T type;
	};
	template<typename T,size_t Size>
	struct remove_extent<T[Size]> {
		typedef T type;
	};
	template<typename T>
	struct remove_extent<T[]> {
		typedef T type;
	};

	// add_const
	template<typename T>
	struct add_const {
		typedef const T type;
	};
	template<typename T>
	struct add_const<const T> {
		typedef T type;
	};
	// add_volatile
	template<typename T>
	struct add_volatile {
		typedef volatile T type;
	};
	template<typename T>
	struct add_volatile<volatile T> {
		typedef T type;
	};
	// add_cv
	template<typename T>
	struct add_cv {
		typedef typename add_volatile<typename add_const<T>::type>::type type;
	};
	// add_pointer
	template<typename T>
	struct add_pointer {
		typedef typename remove_reference<T>::type* type;
	};

	// enable_if
	template<bool,typename T = void>
	struct enable_if {
	};
	template<typename T>
	struct enable_if<true,T> {
		typedef T type;
	};

	template<typename Base,typename Derived>
	struct is_base_of {
		static const bool value = __is_base_of(Base,Derived);
	};

	// convertible
	template<class From,class To>
	struct is_convertible {
		void check() {
			To y A_UNUSED = _x;
		}
		From _x;
	};
	template<class From,class To>
	struct is_const_convertible {
		void check() {
			To y A_UNUSED = const_cast<To>(_x);
		}
		From _x;
	};
	template<class From,class To>
	struct is_static_convertible {
		void check() {
			To y A_UNUSED = static_cast<To>(_x);
		}
		From _x;
	};

	template<class Concept>
	inline void function_requires() {
		void (Concept::*_x)() A_UNUSED = &Concept::check;
	}
}
