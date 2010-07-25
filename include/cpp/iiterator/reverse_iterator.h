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

#ifndef REVERSE_ITERATOR_H_
#define REVERSE_ITERATOR_H_

#include <stddef.h>
#include <iterator>

namespace std {
	template<class Iterator>
	class reverse_iterator {
	protected:
		Iterator _it;
	public:
		typedef Iterator iterator_type;
		typedef typename std::iterator_traits<Iterator>::value_type value_type;
		typedef typename iterator_traits<Iterator>::difference_type difference_type;
		typedef typename iterator_traits<Iterator>::reference reference;
		typedef typename iterator_traits<Iterator>::pointer pointer;
		reverse_iterator();
		explicit reverse_iterator(Iterator x);
		Iterator base() const; // explicit
		reference operator *() const;
		pointer operator ->() const;
		reverse_iterator& operator ++();
		reverse_iterator operator ++(int);
		reverse_iterator& operator --();
		reverse_iterator operator --(int);
		reverse_iterator operator +(difference_type n) const;
		reverse_iterator& operator +=(difference_type n);
		reverse_iterator operator -(difference_type n) const;
		reverse_iterator& operator -=(difference_type n);
		reference operator [](difference_type n) const;
	};

	template<class Iterator1,class Iterator2>
	bool operator ==(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y);
	template<class Iterator1,class Iterator2>
	bool operator <(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y);
	template<class Iterator1,class Iterator2>
	bool operator !=(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y);
	template<class Iterator1,class Iterator2>
	bool operator >(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y);
	template<class Iterator1,class Iterator2>
	bool operator >=(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y);
	template<class Iterator1,class Iterator2>
	bool operator <=(const reverse_iterator<Iterator1>& x,const reverse_iterator<Iterator2>& y);
#if 0
	template<class Iterator1,class Iterator2>
	typename reverse_iterator<Iterator1>::difference_type operator -(const reverse_iterator<
			Iterator1>& x,const reverse_iterator<Iterator2>& y);
	template<class Iterator>
	reverse_iterator<Iterator> operator +(typename reverse_iterator<Iterator>::difference_type n,
			const reverse_iterator<Iterator>& x);
#endif
}

#include "../../../lib/cpp/src/iiterator/reverse_iterator.cc"

#endif /* REVERSE_ITERATOR_H_ */
