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

namespace esc {
	// returns the iterator-category for the given iterator
	template<typename Iter>
	static inline typename iterator_traits<Iter>::iterator_category iterator_category(const Iter&) {
		return typename iterator_traits<Iter>::iterator_category();
	}

	// helper for advance
	template<class RandomAccessIterator,class Distance>
	static inline void advance(RandomAccessIterator& i,Distance n,random_access_iterator_tag) {
		i += n;
	}
	template<class InputIterator,class Distance>
	static inline void advance(InputIterator& i,Distance n,input_iterator_tag) {
		while(n-- > 0)
			++i;
	}

	template<class InputIterator,class Distance>
	inline void advance(InputIterator& i,Distance n) {
		advance(i,n,iterator_category(i));
	}

	// helper for distance
	template<class RandomAccessIterator>
	static inline typename iterator_traits<RandomAccessIterator>::difference_type distance(
			RandomAccessIterator first,RandomAccessIterator last,random_access_iterator_tag) {
		return last - first;
	}
	template<class InputIterator>
	static typename iterator_traits<InputIterator>::difference_type distance(
			InputIterator first,InputIterator last,input_iterator_tag) {
		typename iterator_traits<InputIterator>::difference_type diff = 0;
		while(first != last) {
			++first;
			++diff;
		}
		return diff;
	}

	template<class InputIterator>
	inline typename iterator_traits<InputIterator>::difference_type distance(
			InputIterator first,InputIterator last) {
		return distance(first,last,iterator_category(first));
	}
}
