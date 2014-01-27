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
#include <bits/c++config.h>
#include <algorithm>
#include <assert.h>

namespace std {
	template<class T>
	class unique_ptr {
		template<class T2>
		friend class unique_ptr;

	public:
		typedef T* pointer;
		typedef T element_type;

		// 20.7.1.2.1, constructors
		constexpr unique_ptr() noexcept
			: _ptr() {
		}
		explicit unique_ptr(pointer p) noexcept
			: _ptr(p) {
		}
		unique_ptr(unique_ptr&& u) noexcept
			: _ptr(u.release()) {
		}
		template<class T2>
		unique_ptr(unique_ptr<T2>&& u) noexcept
			: _ptr(u.release()) {
		}

		// 20.7.1.2.2, destructor
		~unique_ptr() {
			delete _ptr;
		}

		// 20.7.1.2.3, assignment
		unique_ptr& operator=(unique_ptr&& u) noexcept {
			if(_ptr != u._ptr)
				reset(u.release());
			return *this;
		}
		template<class U>
		unique_ptr& operator=(unique_ptr<U>&& u) noexcept {
			if(_ptr != u._ptr)
				reset(u.release());
			return *this;
		}

		// 20.7.1.2.4, observers
		T& operator*() const {
			assert(get() != nullptr);
			return *get();
		}
		pointer operator->() const noexcept {
			assert(get() != nullptr);
			return get();
		}
		pointer get() const noexcept {
			return _ptr;
		}
		explicit operator bool() const noexcept {
			return get() != nullptr;
		}

		// 20.7.1.2.5 modifiers
		pointer release() noexcept {
			pointer p = _ptr;
			_ptr = nullptr;
			return p;
		}
		void reset(pointer p = pointer()) noexcept {
			delete _ptr;
			_ptr = p;
		}
		void swap(unique_ptr& u) noexcept {
			std::swap(_ptr,u._ptr);
		}

		// disable copy from lvalue
		unique_ptr(const unique_ptr&) = delete;
		unique_ptr& operator=(const unique_ptr&) = delete;

	private:
		T *_ptr;
	};

	template<class T>
	class unique_ptr<T[]> {
		template<class T2>
		friend class unique_ptr;

	public:
		typedef T* pointer;
		typedef T element_type;

		// 20.7.1.3.1, constructors
		constexpr unique_ptr() noexcept
			: _ptr() {
		}
		explicit unique_ptr(pointer p) noexcept
			: _ptr(p) {
		}
		unique_ptr(unique_ptr&& u) noexcept
			: _ptr(u.release()) {
		}

		// destructor
		~unique_ptr() {
			delete[] _ptr;
		}

		// assignment
		unique_ptr& operator=(unique_ptr&& u) noexcept {
			if(_ptr != u._ptr)
				reset(u.release());
			return *this;
		}

		// 20.7.1.3.2, observers
		T& operator[](size_t i) const {
			assert(get() != nullptr);
			return _ptr[i];
		}
		pointer get() const noexcept {
			return _ptr;
		}
		explicit operator bool() const noexcept {
			return get() != nullptr;
		}

		// 20.7.1.3.3 modifiers
		pointer release() noexcept {
			pointer p = _ptr;
			_ptr = nullptr;
			return p;
		}
		void reset(pointer p = pointer()) noexcept {
			delete[] _ptr;
			_ptr = p;
		}
		template<class T2>
		void reset(T2) = delete;
		void swap(unique_ptr& u) noexcept {
			std::swap(_ptr,u._ptr);
		}

		// disable copy from lvalue
		unique_ptr(const unique_ptr&) = delete;
		unique_ptr& operator=(const unique_ptr&) = delete;

	private:
		T *_ptr;
	};

	template<class T>
	void swap(unique_ptr<T>& x, unique_ptr<T>& y) noexcept {
		x.swap(y);
	}

	template<class T1,class T2>
	inline bool operator==(const unique_ptr<T1>& x, const unique_ptr<T2>& y) {
		return x.get() == y.get();
	}
	template<class T1, class T2>
	inline bool operator!=(const unique_ptr<T1>& x, const unique_ptr<T2>& y) {
		return !(x == y);
	}
	template<class T1, class T2>
	inline bool operator<(const unique_ptr<T1>& x, const unique_ptr<T2>& y) {
		return x.get() < y.get();
	}
	template<class T1, class T2>
	inline bool operator<=(const unique_ptr<T1>& x, const unique_ptr<T2>& y) {
		return !(x > y);
	}
	template<class T1, class T2>
	inline bool operator>(const unique_ptr<T1>& x, const unique_ptr<T2>& y) {
		return x.get() > y.get();
	}
	template<class T1, class T2>
	inline bool operator>=(const unique_ptr<T1>& x, const unique_ptr<T2>& y) {
		return !(x < y);
	}
}
