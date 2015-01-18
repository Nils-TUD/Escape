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

#include <iterator>
#include <stddef.h>

namespace std {
	template<class Iterator>
	class reverse_iterator: public iterator<
			typename iterator_traits<Iterator>::iterator_category,
			typename iterator_traits<Iterator>::value_type,
			typename iterator_traits<Iterator>::difference_type,
			typename iterator_traits<Iterator>::pointer,
			typename iterator_traits<Iterator>::reference> {
	protected:
		Iterator _it;
	public:
		typedef Iterator iterator_type;
		typedef typename std::iterator_traits<Iterator>::value_type value_type;
		typedef typename iterator_traits<Iterator>::difference_type difference_type;
		typedef typename iterator_traits<Iterator>::reference reference;
		typedef typename iterator_traits<Iterator>::pointer pointer;
		reverse_iterator()
			: _it() {
		}
		explicit reverse_iterator(Iterator x)
			: _it(x) {
		}
		Iterator base() const {
			return _it;
		}
		reference operator *() const {
			Iterator prev(_it);
			--prev;
			return (*prev);
		}
		pointer operator ->() const {
			return &(operator*());
		}
		reverse_iterator& operator ++() {
			--_it;
			return *this;
		}
		reverse_iterator operator ++(int) {
			reverse_iterator<Iterator> tmp = *this;
			--_it;
			return tmp;
		}
		reverse_iterator& operator --() {
			++_it;
			return *this;
		}
		reverse_iterator operator --(int) {
			reverse_iterator<Iterator> tmp = *this;
			++_it;
			return tmp;
		}
		reverse_iterator operator +(difference_type n) const {
			return reverse_iterator<Iterator>(_it - n);
		}
		reverse_iterator& operator +=(difference_type n) {
			_it -= n;
			return *this;
		}
		reverse_iterator operator -(difference_type n) const {
			return reverse_iterator<Iterator>(_it + n);
		}
		reverse_iterator& operator -=(difference_type n) {
			_it += n;
			return *this;
		}
		reference operator [](difference_type n) const {
			return (*(*this + n));
		}
	};

	template<class Iterator1,class Iterator2>
	inline bool operator ==(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y) {
		return x.base() == y.base();
	}
	template<class Iterator1,class Iterator2>
	inline bool operator <(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y) {
		return x.base() < y.base();
	}
	template<class Iterator1,class Iterator2>
	inline bool operator !=(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y) {
		return x.base() != y.base();
	}
	template<class Iterator1,class Iterator2>
	inline bool operator >(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y) {
		return x.base() > y.base();
	}
	template<class Iterator1,class Iterator2>
	inline bool operator >=(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y) {
		return x.base() >= y.base();
	}
	template<class Iterator1,class Iterator2>
	inline bool operator <=(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y) {
		return x.base() <= y.base();
	}

	template<class Iterator1,class Iterator2>
	inline typename reverse_iterator<Iterator1>::difference_type operator -(
			const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y) {
		return y.base() - x.base();
	}
	template<class Iterator>
	inline reverse_iterator<Iterator> operator +(typename reverse_iterator<Iterator>::difference_type n,
			const reverse_iterator<Iterator>& x) {
		return reverse_iterator<Iterator>(x.base() - n);
	}
}
