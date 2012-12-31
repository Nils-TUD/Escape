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

#pragma once

#include <stddef.h>

namespace std {
	struct input_iterator_tag {
	};
	struct output_iterator_tag {
	};
	struct forward_iterator_tag : public input_iterator_tag {
	};
	struct bidirectional_iterator_tag : public forward_iterator_tag {
	};
	struct random_access_iterator_tag : public bidirectional_iterator_tag {
	};

	template<class Category,class T,class Distance = ptrdiff_t,class Pointer = T*,
			class Reference = T&>
	struct iterator {
		typedef T value_type;
		typedef Distance difference_type;
		typedef Pointer pointer;
		typedef Reference reference;
		typedef Category iterator_category;
	};

	template<typename Iter>
	struct iterator_traits {
		typedef typename Iter::iterator_category iterator_category;
		typedef typename Iter::value_type value_type;
		typedef typename Iter::difference_type difference_type;
		typedef typename Iter::pointer pointer;
		typedef typename Iter::reference reference;
	};

	template<typename Iter>
	struct iterator_traits<Iter*> {
		typedef random_access_iterator_tag iterator_category;
		typedef Iter value_type;
		typedef ptrdiff_t difference_type;
		typedef Iter* pointer;
		typedef Iter& reference;
	};

	template<typename Iter>
	struct iterator_traits<const Iter*> {
		typedef random_access_iterator_tag iterator_category;
		typedef Iter value_type;
		typedef ptrdiff_t difference_type;
		typedef const Iter* pointer;
		typedef const Iter& reference;
	};

#if 0
	template<class Iterator>
	bool operator==(const reverse_iterator<Iterator>& x,const reverse_iterator<Iterator>& y);
	template<class Iterator>
	bool operator<(const reverse_iterator<Iterator>& x,const reverse_iterator<Iterator>& y);
	template<class Iterator>
	bool operator!=(const reverse_iterator<Iterator>& x,const reverse_iterator<Iterator>& y);
	template<class Iterator>
	bool operator>(const reverse_iterator<Iterator>& x,const reverse_iterator<Iterator>& y);
	template<class Iterator>
	bool operator>=(const reverse_iterator<Iterator>& x,const reverse_iterator<Iterator>& y);
	template<class Iterator>
	bool operator<=(const reverse_iterator<Iterator>& x,const reverse_iterator<Iterator>& y);

	template<class Iterator>
	typename reverse_iterator<Iterator>::difference_type operator-(
			const reverse_iterator<Iterator>& x,const reverse_iterator<Iterator>& y);

	template<class Iterator>
	reverse_iterator<Iterator> operator+(typename reverse_iterator<Iterator>::difference_type n,
			const reverse_iterator<Iterator>& x);

	template<class Container>
	class back_insert_iterator;
	template<class Container>
	back_insert_iterator<Container> back_inserter(Container & x);
	template<class Container>
	class front_insert_iterator;
	template<class Container>
	front_insert_iterator<Container> front_inserter(Container & x);
	template<class Container>
	class insert_iterator;
	template<class Container,class Iterator>
	insert_iterator<Container> inserter(Container & x,Iterator i);

	template<class T,class charT = char,class traits = char_traits<charT> ,
			class Distance = ptrdiff_t>
	class istream_iterator;
	template<class T,class charT,class traits,class Distance>
	bool operator==(const istream_iterator<T,charT,traits,Distance>& x,
			const istream_iterator<T,charT,traits,Distance>& y);
	template<class T,class charT,class traits,class Distance>
	bool operator!=(const istream_iterator<T,charT,traits,Distance>& x,
			const istream_iterator<T,charT,traits,Distance>& y);
	template<class T,class charT = char,class traits = char_traits<charT> >
	class ostream_iterator;

	template<class charT,class traits = char_traits<charT> >
	class istreambuf_iterator;
	template<class charT,class traits>
	bool operator==(const istreambuf_iterator<charT,traits>& a,
			const istreambuf_iterator<charT,traits>& b);
	template<class charT,class traits>
	bool operator !=(const istreambuf_iterator<charT,traits>& a,
			const istreambuf_iterator<charT,traits>& b);
	template<class charT,class traits = char_traits<charT> >
	class ostreambuf_iterator;
#endif
}
