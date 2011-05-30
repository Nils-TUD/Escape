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

namespace std {
	template<class T1,class T2>
	static bool defEqual(T1 a,T2 b) {
		return a == b;
	}
	template<class T1,class T2>
	static bool defLessThan(T1 a,T2 b) {
		return a < b;
	}

	template<class InputIterator,class Function>
	Function for_each(InputIterator first,InputIterator last,Function f) {
		for(; first != last; ++first)
			f(*first);
		return f;
	}

	template<class InputIterator,class T>
	InputIterator find(InputIterator first,InputIterator last,const T& value) {
		for(; first != last; ++first) {
			if(*first == value)
				break;
		}
		return first;
	}

	template<class InputIterator,class Predicate>
	InputIterator find_if(InputIterator first,InputIterator last,Predicate pred) {
		for(; first != last; ++first) {
			if(pred(*first))
				break;
		}
		return first;
	}

	template<class ForwardIterator1,class ForwardIterator2>
	inline ForwardIterator1 find_end(ForwardIterator1 first1,ForwardIterator1 last1,
			ForwardIterator2 first2,ForwardIterator2 last2) {
	    typedef typename std::iterator_traits<ForwardIterator1>::value_type T1;
	    typedef typename std::iterator_traits<ForwardIterator2>::value_type T2;
		return find_end(first1,last1,first2,last2,defEqual<T1,T2>);
	}
	template<class ForwardIterator1,class ForwardIterator2,class Predicate>
	ForwardIterator1 find_end(ForwardIterator1 first1,ForwardIterator1 last1,
			ForwardIterator2 first2,ForwardIterator2 last2,Predicate pred) {
		ForwardIterator1 it1,limit,ret;
		ForwardIterator2 it2;

		limit = first1;
		advance(limit,1 + distance(first1,last1) - distance(first2,last2));
		ret = last1;

		while(first1 != limit) {
			it1 = first1;
			it2 = first2;
			while(pred(*it1,*it2)) {
				++it1;
				++it2;
				if(it2 == last2) {
					ret = first1;
					break;
				}
			}
			++first1;
		}
		return ret;
	}

	template<class ForwardIterator1,class ForwardIterator2>
	inline ForwardIterator1 find_first_of(ForwardIterator1 first1,ForwardIterator1 last1,
			ForwardIterator2 first2,ForwardIterator2 last2) {
	    typedef typename std::iterator_traits<ForwardIterator1>::value_type T1;
	    typedef typename std::iterator_traits<ForwardIterator2>::value_type T2;
		return find_first_of(first1,last1,first2,last2,defEqual<T1,T2>);
	}
	template<class ForwardIterator1,class ForwardIterator2,class BinaryPredicate>
	ForwardIterator1 find_first_of(ForwardIterator1 first1,ForwardIterator1 last1,
			ForwardIterator2 first2,ForwardIterator2 last2,BinaryPredicate pred) {
		for(; first1 != last1; ++first1) {
			for(ForwardIterator2 it2 = first2; it2 != last2; ++it2) {
				if(pred(*first1,*it2))
					return first1;
			}
		}
		return last1;
	}

	template<class InputIterator,class T>
	typename iterator_traits<InputIterator>::difference_type count(
			InputIterator first,InputIterator last,const T& value) {
		typename iterator_traits<InputIterator>::difference_type count = 0;
		for(; first != last; ++first) {
			if(*first == value)
				count++;
		}
		return count;
	}
	template<class InputIterator,class Predicate>
	typename iterator_traits<InputIterator>::difference_type count_if(
			InputIterator first,InputIterator last,Predicate pred) {
		typename iterator_traits<InputIterator>::difference_type count = 0;
		for(; first != last; ++first) {
			if(pred(*first))
				count++;
		}
		return count;
	}

	template<class InputIterator1,class InputIterator2>
	pair<InputIterator1,InputIterator2> mismatch(InputIterator1 first1,
			InputIterator1 last1,InputIterator2 first2) {
		while(first1 != last1) {
			if(*first1 != *first2)
				break;
			++first1;
			++first2;
		}
		return make_pair(first1,first2);
	}
	template<class InputIterator1,class InputIterator2,class BinaryPredicate>
	pair<InputIterator1,InputIterator2> mismatch(InputIterator1 first1,
			InputIterator1 last1,InputIterator2 first2,BinaryPredicate pred) {
		while(first1 != last1) {
			if(!pred(*first1,*first2))
				break;
			++first1;
			++first2;
		}
		return make_pair(first1,first2);
	}

	template<class InputIterator1,class InputIterator2>
	inline bool equal(InputIterator1 first1,InputIterator1 last1,InputIterator2 first2) {
	    typedef typename std::iterator_traits<InputIterator1>::value_type T1;
	    typedef typename std::iterator_traits<InputIterator2>::value_type T2;
		return equal(first1,last1,first2,defEqual<T1,T2>);
	}
	template<class InputIterator1,class InputIterator2,class BinaryPredicate>
	bool equal(InputIterator1 first1,InputIterator1 last1,InputIterator2 first2,
			BinaryPredicate pred) {
		for(; first1 != last1; ++first1, ++first2) {
			if(!(*first1 == *first2))
				return false;
		}
		return true;
	}

	template<class ForwardIterator1,class ForwardIterator2>
	ForwardIterator1 search(ForwardIterator1 first1,ForwardIterator1 last1,ForwardIterator2 first2,
			ForwardIterator2 last2) {
	    typedef typename std::iterator_traits<ForwardIterator1>::value_type T1;
	    typedef typename std::iterator_traits<ForwardIterator2>::value_type T2;
		return search(first1,last1,first2,last2,defEqual<T1,T2>);
	}
	template<class ForwardIterator1,class ForwardIterator2,class BinaryPredicate>
	ForwardIterator1 search(ForwardIterator1 first1,ForwardIterator1 last1,ForwardIterator2 first2,
			ForwardIterator2 last2,BinaryPredicate pred) {
		ForwardIterator1 it1,limit;
		ForwardIterator2 it2;

		limit = first1;
		advance(limit,1 + distance(first1,last1) - distance(first2,last2));

		while(first1 != limit) {
			it1 = first1;
			it2 = first2;
			while(pred(*it1,*it2)) {
				++it1;
				++it2;
				if(it2 == last2)
					return first1;
			}
			++first1;
		}
		return last1;
	}

	template<class InputIterator,class OutputIterator>
	OutputIterator copy(InputIterator first,InputIterator last,OutputIterator result) {
		for(; first != last; ++result, ++first)
			*result = *first;
		return result;
	}

	template<class T>
	inline void swap(T& a,T& b) {
		T tmp(a);
		a = b;
		b = tmp;
	}

	template<class ForwardIterator1,class ForwardIterator2>
	inline void iter_swap(ForwardIterator1 a,ForwardIterator2 b) {
		swap(*a,*b);
	}

	template<class InputIterator,class OutputIterator,class UnaryOperator>
	OutputIterator transform(InputIterator first1,InputIterator last1,OutputIterator result,
			UnaryOperator op) {
		for(; first1 != last1; ++result, ++first1)
			*result = op(*first1);
		return result;
	}
	template<class InputIterator1,class InputIterator2,class OutputIterator,class BinaryOperator>
	OutputIterator transform(InputIterator1 first1,InputIterator1 last1,InputIterator2 first2,
			OutputIterator result,BinaryOperator binary_op) {
		for(; first1 != last1; ++result, ++first1, ++first2)
			*result = binary_op(*first1,*first2);
		return result;
	}

	template<class ForwardIterator,class T>
	void replace(ForwardIterator first,ForwardIterator last,const T& old_value,const T& new_value) {
		for(; first != last; ++first) {
			if(*first == old_value)
				*first = new_value;
		}
	}
	template<class ForwardIterator,class Predicate,class T>
	void replace_if(ForwardIterator first,ForwardIterator last,Predicate pred,const T& new_value) {
		for(; first != last; ++first) {
			if(pred(*first))
				*first = new_value;
		}
	}

	template<class ForwardIterator,class T>
	void fill(ForwardIterator first,ForwardIterator last,const T& value) {
		while(first != last)
			*first++ = value;
	}
	template<class OutputIterator,class Size,class T>
	void fill_n(OutputIterator first,Size n,const T& value) {
		while(n-- > 0)
			*first++ = value;
	}

	template<class ForwardIterator,class Generator>
	void generate(ForwardIterator first,ForwardIterator last,Generator gen) {
		while(first != last)
			*first++ = gen();
	}
	template<class OutputIterator,class Size,class Generator>
	void generate_n(OutputIterator first,Size n,Generator gen) {
		while(n-- > 0)
			*first++ = gen();
	}

	template<class ForwardIterator,class T>
	ForwardIterator remove(ForwardIterator first,ForwardIterator last,const T& value) {
		ForwardIterator result = first;
		for(; first != last; ++first)
			if(!(*first == value))
				*result++ = *first;
		return result;
	}
	template<class ForwardIterator,class Predicate>
	ForwardIterator remove_if(ForwardIterator first,ForwardIterator last,Predicate pred) {
		ForwardIterator result = first;
		for(; first != last; ++first)
			if(!pred(*first))
				*result++ = *first;
		return result;
	}

	template<class BidirectionalIterator>
	void reverse(BidirectionalIterator first,BidirectionalIterator last) {
		while((first != last) && (first != --last))
			swap(*first++,*last);
	}
	template<class BidirectionalIterator,class OutputIterator>
	OutputIterator reverse_copy(BidirectionalIterator first,BidirectionalIterator last,
			OutputIterator result) {
		while(first != last)
			*result++ = *--last;
		return result;
	}

	template<class RandAccIt,class Compare>
	static RandAccIt divide(RandAccIt ileft,RandAccIt ipiv,Compare comp) {
		RandAccIt i = ileft;
		RandAccIt j = ipiv - 1;
		do {
			// right until the element is > piv
			while(!comp(*ipiv,*i) && i < ipiv)
				++i;
			// left until the element is < piv
			while(!comp(*j,*ipiv) && j > ileft)
				--j;

			// swap
			if(i < j) {
				swap(*i,*j);
				++i;
				--j;
			}
		}
		while(i < j);

		// swap piv with element i
		if(comp(*ipiv,*i))
			swap(*i,*ipiv);
		return i;
	}
	template<class RandAccIt,class Compare>
	static void qsort(RandAccIt left,RandAccIt right,Compare comp) {
		// TODO someday we should provide a better implementation which uses another sort-algo
		// for small arrays, don't uses recursion and so on
		if(left < right) {
			RandAccIt i = divide(left,right,comp);
			qsort(left,i - 1,comp);
			qsort(i + 1,right,comp);
		}
	}

	template<class RandomAccessIterator>
	inline void sort(RandomAccessIterator first,RandomAccessIterator last) {
	    typedef typename std::iterator_traits<RandomAccessIterator>::value_type T;
		sort(first,last,defLessThan<T,T>);
	}
	template<class RandomAccessIterator,class Compare>
	void sort(RandomAccessIterator first,RandomAccessIterator last,Compare comp) {
		qsort(first,last - 1,comp);
	}

	template<class ForwardIterator,class T>
	ForwardIterator lower_bound(ForwardIterator first,ForwardIterator last,const T& value) {
		return lower_bound(first,last,value,defLessThan<T,T>);
	}
	template<class ForwardIterator,class T,class Compare>
	ForwardIterator lower_bound(ForwardIterator first,ForwardIterator last,const T& value,
			Compare comp) {
		ForwardIterator it;
		typename iterator_traits<ForwardIterator>::difference_type count,step;
		count = distance(first,last);
		while(count > 0) {
			it = first;
			step = count / 2;
			advance(it,step);
			if(comp(*it,value)) {
				first = ++it;
				count -= step + 1;
			}
			else
				count = step;
		}
		return first;
	}

	template<class ForwardIterator,class T>
	ForwardIterator upper_bound(ForwardIterator first,ForwardIterator last,const T& value) {
		return upper_bound(first,last,value,defLessThan<T,T>);
	}
	template<class ForwardIterator,class T,class Compare>
	ForwardIterator upper_bound(ForwardIterator first,ForwardIterator last,const T& value,
			Compare comp) {
		ForwardIterator it;
		typename iterator_traits<ForwardIterator>::difference_type count,step;
		count = distance(first,last);
		while(count > 0) {
			it = first;
			step = count / 2;
			advance(it,step);
			if(!comp(value,*it)) {
				first = ++it;
				count -= step + 1;
			}
			else
				count = step;
		}
		return first;
	}

	template<class ForwardIterator,class T>
	bool binary_search(ForwardIterator first,ForwardIterator last,const T& value) {
		first = lower_bound(first,last,value);
		return (first != last && !(value < *first));
	}
	template<class ForwardIterator,class T,class Compare>
	bool binary_search(ForwardIterator first,ForwardIterator last,const T& value,Compare comp) {
		first = lower_bound(first,last,value,comp);
		return (first != last && !comp(value,*first));
	}

	template<class InputIterator1,class InputIterator2,class OutputIterator>
	OutputIterator merge(InputIterator1 first1,InputIterator1 last1,InputIterator2 first2,
			InputIterator2 last2,OutputIterator result) {
		while(true) {
			*result++ = (*first2 < *first1) ? *first2++ : *first1++;
			if(first1 == last1)
				return copy(first2,last2,result);
			if(first2 == last2)
				return copy(first1,last1,result);
		}
	}
	template<class InputIterator1,class InputIterator2,class OutputIterator,class Compare>
	OutputIterator merge(InputIterator1 first1,InputIterator1 last1,InputIterator2 first2,
			InputIterator2 last2,OutputIterator result,Compare comp) {
		while(true) {
			*result++ = comp(*first2,*first1) ? *first2++ : *first1++;
			if(first1 == last1)
				return copy(first2,last2,result);
			if(first2 == last2)
				return copy(first1,last1,result);
		}
	}

	template<class InputIterator1,class InputIterator2>
	bool includes(InputIterator1 first1,InputIterator1 last1,InputIterator2 first2,
			InputIterator2 last2) {
		typedef typename std::iterator_traits<InputIterator1>::value_type T1;
		typedef typename std::iterator_traits<InputIterator2>::value_type T2;
		return includes(first1,last1,first2,last2,defLessThan<T1,T2>);
	}
	template<class InputIterator1,class InputIterator2,class Compare>
	bool includes(InputIterator1 first1,InputIterator1 last1,InputIterator2 first2,
			InputIterator2 last2,Compare comp) {
		while(first1 != last1) {
			if(comp(*first2,*first1))
				break;
			else if(comp(*first1,*first2))
				++first1;
			else {
				++first1;
				++first2;
			}
			if(first2 == last2)
				return true;
		}
		return false;
	}

	template<class T>
	const T& min(const T& a,const T& b) {
		return (a < b) ? a : b;
	}
	template<class T,class Compare>
	const T& min(const T& a,const T& b,Compare comp) {
		return comp(a,b) ? a : b;
	}

	template<class T>
	const T& max(const T& a,const T& b) {
		return (a < b) ? b : a;
	}
	template<class T,class Compare>
	const T& max(const T& a,const T& b,Compare comp) {
		return comp(a,b) ? b : a;
	}

	template<class ForwardIterator>
	ForwardIterator min_element(ForwardIterator first,ForwardIterator last) {
		ForwardIterator lowest = first;
		if(first == last)
			return last;
		while(++first != last)
			if(*first < *lowest)
				lowest = first;
		return lowest;
	}
	template<class ForwardIterator,class Compare>
	ForwardIterator min_element(ForwardIterator first,ForwardIterator last,Compare comp) {
		ForwardIterator lowest = first;
		if(first == last)
			return last;
		while(++first != last)
			if(comp(*first,*lowest))
				lowest = first;
		return lowest;
	}

	template<class ForwardIterator>
	ForwardIterator max_element(ForwardIterator first,ForwardIterator last) {
		ForwardIterator largest = first;
		if(first == last)
			return last;
		while(++first != last)
			if(*largest < *first)
				largest = first;
		return largest;
	}
	template<class ForwardIterator,class Compare>
	ForwardIterator max_element(ForwardIterator first,ForwardIterator last,Compare comp) {
		ForwardIterator largest = first;
		if(first == last)
			return last;
		while(++first != last)
			if(comp(*largest < *first))
				largest = first;
		return largest;
	}

	template<class InputIterator1,class InputIterator2>
	bool lexicographical_compare(InputIterator1 first1,InputIterator1 last1,InputIterator2 first2,
			InputIterator2 last2) {
	    typedef typename std::iterator_traits<InputIterator1>::value_type T1;
	    typedef typename std::iterator_traits<InputIterator2>::value_type T2;
		return lexicographical_compare(first1,last1,first2,last2,defLessThan<T1,T2>);
	}
	template<class InputIterator1,class InputIterator2,class Compare>
	bool lexicographical_compare(InputIterator1 first1,InputIterator1 last1,InputIterator2 first2,
			InputIterator2 last2,Compare comp) {
		while(first1 != last1) {
			if(comp(*first2,*first1) || first2 == last2)
				return false;
			else if(comp(*first1,*first2))
				return true;
			first1++;
			first2++;
		}
		return (first2 != last2);
	}
}
