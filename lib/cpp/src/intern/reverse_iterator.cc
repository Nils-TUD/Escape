/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <stddef.h>
#include <intern/reverse_iterator.h>

namespace std {
	// === constructors ===
	template<class Iterator>
	inline reverse_iterator<Iterator>::reverse_iterator() : _it() {
	}
	template<class Iterator>
	inline reverse_iterator<Iterator>::reverse_iterator(Iterator x) : _it(x) {
	}

	// === getting position ===
	template<class Iterator>
	inline Iterator reverse_iterator<Iterator>::base() const {
		return _it;
	}
	template<class Iterator>
	inline typename reverse_iterator<Iterator>::reference reverse_iterator<Iterator>::operator*() const {
		Iterator prev(_it);
		--prev;
		return (*prev);
	}
	template<class Iterator>
	inline typename reverse_iterator<Iterator>::pointer reverse_iterator<Iterator>::operator->() const {
		return &(operator*());
	}

	// === operator ++,-- ===
	template<class Iterator>
	inline reverse_iterator<Iterator>& reverse_iterator<Iterator>::operator++() {
		--_it;
		return *this;
	}
	template<class Iterator>
	inline reverse_iterator<Iterator> reverse_iterator<Iterator>::operator++(int) {
		reverse_iterator<Iterator> tmp = *this;
		--_it;
		return tmp;
	}
	template<class Iterator>
	inline reverse_iterator<Iterator>& reverse_iterator<Iterator>::operator--() {
		++_it;
		return *this;
	}
	template<class Iterator>
	inline reverse_iterator<Iterator> reverse_iterator<Iterator>::operator--(int) {
		reverse_iterator<Iterator> tmp = *this;
		++_it;
		return tmp;
	}

	// === operator +,- ===
	template<class Iterator>
	inline reverse_iterator<Iterator> reverse_iterator<Iterator>::operator+(difference_type n) const {
		return reverse_iterator<Iterator>(_it - n);
	}
	template<class Iterator>
	inline reverse_iterator<Iterator>& reverse_iterator<Iterator>::operator+=(difference_type n) {
		_it -= n;
		return *this;
	}
	template<class Iterator>
	inline reverse_iterator<Iterator> reverse_iterator<Iterator>::operator-(difference_type n) const {
		return reverse_iterator<Iterator>(_it + n);
	}
	template<class Iterator>
	inline reverse_iterator<Iterator>& reverse_iterator<Iterator>::operator-=(difference_type n) {
		_it += n;
		return *this;
	}

	// === operator [] ===
	template<class Iterator>
	inline typename reverse_iterator<Iterator>::reference reverse_iterator<Iterator>::operator[](
			difference_type n) const {
		return (*(*this + n));
	}

	// === freestanding operators ===
	template<class Iterator1,class Iterator2>
	bool operator ==(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y) {
		return x.base() == y.base();
	}
	template<class Iterator1,class Iterator2>
	bool operator <(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y) {
		return x.base() < y.base();
	}
	template<class Iterator1,class Iterator2>
	bool operator !=(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y) {
		return x.base() != y.base();
	}
	template<class Iterator1,class Iterator2>
	bool operator >(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y) {
		return x.base() > y.base();
	}
	template<class Iterator1,class Iterator2>
	bool operator >=(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y) {
		return x.base() >= y.base();
	}
	template<class Iterator1,class Iterator2>
	bool operator <=(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y) {
		return x.base() <= y.base();
	}
}
