/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

namespace std {
	template<class Y>
	inline auto_ptr_ref<Y>::auto_ptr_ref(Y *p)
		: _p(p) {
	}

	template<class X>
	inline auto_ptr<X>::auto_ptr(X* p) throw()
		: _p(p) {
	}

	template<class X>
	inline auto_ptr<X>::auto_ptr(auto_ptr<X>& ptr) throw()
		: _p(nullptr) {
		_p = ptr.release();
	}

	template<class X>
	template<class Y>
	inline auto_ptr<X>::auto_ptr(auto_ptr<Y>& ptr) throw()
		: _p(nullptr) {
		_p = ptr.release();
	}

	template<class X>
	inline auto_ptr<X>& auto_ptr<X>::operator =(auto_ptr<X>& ptr) throw() {
		reset(ptr.release());
		return *this;
	}

	template<class X>
	template<class Y>
	inline auto_ptr<X>& auto_ptr<X>::operator =(auto_ptr<Y>& ptr) throw() {
		reset(ptr.release());
		return *this;
	}

	template<class X>
	inline auto_ptr<X>::~auto_ptr() throw() {
		reset(nullptr);
	}

	template<class X>
	inline X& auto_ptr<X>::operator *() const throw() {
		return *_p;
	}
	template<class X>
	inline X* auto_ptr<X>::operator ->() const throw() {
		return _p;
	}
	template<class X>
	inline X* auto_ptr<X>::get() const throw() {
		return _p;
	}
	template<class X>
	X* auto_ptr<X>::release() throw() {
		X* p = _p;
		_p = nullptr;
		return p;
	}
	template<class X>
	void auto_ptr<X>::reset(X* p) throw() {
		if(_p != p) {
			delete _p;
			_p = p;
		}
	}

	template<class X>
	inline auto_ptr<X>::auto_ptr(auto_ptr_ref<X> r) throw()
		: _p(r._p) {
	}
	template<class X>
	template<class Y>
	inline auto_ptr<X>::operator auto_ptr_ref<Y>() throw() {
		return auto_ptr_ref<Y>(release());
	}
	template<class X>
	template<class Y>
	inline auto_ptr<X>::operator auto_ptr<Y>() throw() {
		return auto_ptr<Y>(release());
	}
	template<class X>
	auto_ptr<X>& auto_ptr<X>::operator =(auto_ptr_ref<X> r) throw() {
		reset(r._p);
		return *this;
	}

	template<class X>
	inline auto_array<X>::auto_array(X* p) throw()
		: auto_ptr<X>(p) {
	}
	template<class X>
	inline auto_array<X>::~auto_array() throw() {
	}
	template<class X>
	inline X& auto_array<X>::operator [](int index) const throw() {
		return this->_p[index];
	}
	template<class X>
	void auto_array<X>::reset(X* p) throw() {
		if(this->_p != p) {
			delete[] this->_p;
			this->_p = p;
		}
	}
}
