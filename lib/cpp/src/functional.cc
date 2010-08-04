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
	template<class T>
	inline T plus<T>::operator()(const T& x,const T& y) {
		return x + y;
	}
	template <class T>
	inline T minus<T>::operator()(const T& x,const T& y) const {
		return x - y;
	}
	template<class T>
	inline T multiplies<T>::operator()(const T& x,const T& y) const {
		return x * y;
	}
	template<class T>
	inline T divides<T>::operator()(const T& x,const T& y) const {
		return x / y;
	}
	template<class T>
	inline T modulus<T>::operator()(const T& x,const T& y) const {
		return x % y;
	}
	template<class T>
	inline T negate<T>::operator()(const T& x) const {
		return -x;
	}

	template<class T>
	inline bool equal_to<T>::operator()(const T& x,const T& y) const {
		return x == y;
	}
	template<class T>
	inline bool not_equal_to<T>::operator()(const T& x,const T& y) const {
		return x != y;
	}
	template<class T>
	inline bool greater<T>::operator()(const T& x,const T& y) const {
		return x > y;
	}
	template<class T>
	inline bool less<T>::operator()(const T& x,const T& y) const {
		return x < y;
	}
	template<class T>
	inline bool greater_equal<T>::operator()(const T& x,const T& y) const {
		return x >= y;
	}
	template<class T>
	inline bool less_equal<T>::operator()(const T& x,const T& y) const {
		return x <= y;
	}

	template<class T>
	inline bool logical_and<T>::operator()(const T& x,const T& y) const {
		return x && y;
	}
	template<class T>
	inline bool logical_or<T>::operator()(const T& x,const T& y) const {
		return x || y;
	}
	template<class T>
	inline bool logical_not<T>::operator()(const T& x) const {
		return !x;
	}

	template<class Predicate>
	inline unary_negate<Predicate>::unary_negate(const Predicate &pred)
		: fn(pred) {
	}
	template<class Predicate>
	inline bool unary_negate<Predicate>::operator()(const typename Predicate::argument_type& x) const {
		return !fn(x);
	}

	template<class Predicate>
	inline binary_negate<Predicate>::binary_negate(const Predicate &pred)
		: fn(pred) {
	}
	template<class Predicate>
	inline bool binary_negate<Predicate>::operator()(const typename Predicate::first_argument_type& x,
			const typename Predicate::second_argument_type& y) const {
		return !fn(x,y);
	}

	template<class Operation>
	inline binder1st<Operation>::binder1st(const Operation& x,
			const typename Operation::first_argument_type& y)
			: op(x), value(y) {
	}
	template<class Operation>
	inline typename Operation::result_type binder1st<Operation>::operator()(
			const typename Operation::second_argument_type& x) const {
		return op(value,x);
	}

	template<class Operation>
	inline binder2nd<Operation>::binder2nd(const Operation& x,
			const typename Operation::second_argument_type& y)
			: op(x), value(y) {
	}
	template<class Operation>
	inline typename Operation::result_type binder2nd<Operation>::operator()(
			const typename Operation::first_argument_type& x) const {
		return op(x,value);
	}

	template<class Arg,class Result>
	inline pointer_to_unary_function<Arg,Result>::pointer_to_unary_function(Result(*f)(Arg))
		: pfunc(f) {
	}
	template<class Arg,class Result>
	inline Result pointer_to_unary_function<Arg,Result>::operator()(Arg x) const {
		return pfunc(x);
	}

	template<class Arg1,class Arg2,class Result>
	inline pointer_to_binary_function<Arg1,Arg2,Result>::pointer_to_binary_function(
			Result(*f)(Arg1,Arg2))
		: pfunc(f) {
	}
	template<class Arg1,class Arg2,class Result>
	inline Result pointer_to_binary_function<Arg1,Arg2,Result>::operator()(Arg1 x,Arg2 y) const {
		return pfunc(x,y);
	}

	template<class S,class T>
	inline mem_fun_t<S,T>::mem_fun_t(S(T::*p)())
		: pmem(p) {
	}
	template<class S,class T>
	inline S mem_fun_t<S,T>::operator()(T* p) const {
		return (p->*pmem)();
	}

	template<class S,class T,class A>
	inline mem_fun1_t<S,T,A>::mem_fun1_t(S(T::*p)(A))
		: pmem(p) {
	}
	template<class S,class T,class A>
	inline S mem_fun1_t<S,T,A>::operator()(T* p,A x) const {
		return (p->*pmem)(x);
	}

	template<class S,class T>
	inline const_mem_fun_t<S,T>::const_mem_fun_t(S(T::*p)() const)
		: pmem(p) {
	}
	template<class S,class T>
	inline S const_mem_fun_t<S,T>::operator()(T* p) const {
		return (p->*pmem)();
	}

	template<class S,class T,class A>
	inline const_mem_fun1_t<S,T,A>::const_mem_fun1_t(S(T::*p)(A) const)
		: pmem(p) {
	}
	template<class S,class T,class A>
	inline S const_mem_fun1_t<S,T,A>::operator()(T* p,A x) const {
		return (p->*pmem)(x);
	}

	template<class S,class T>
	inline mem_fun_ref_t<S,T>::mem_fun_ref_t(S(T::*p)())
		: pmem(p) {
	}
	template<class S,class T>
	inline S mem_fun_ref_t<S,T>::operator()(T& p) const {
		return (p.*pmem)();
	}

	template<class S,class T,class A>
	inline mem_fun1_ref_t<S,T,A>::mem_fun1_ref_t(S(T::*p)(A))
		: pmem(p) {
	}
	template<class S,class T,class A>
	inline S mem_fun1_ref_t<S,T,A>::operator()(T& p,A x) const {
		return (p.*pmem)(x);
	}

	template<class S,class T>
	inline const_mem_fun_ref_t<S,T>::const_mem_fun_ref_t(S(T::*p)() const)
		: pmem(p) {
	}
	template<class S,class T>
	inline S const_mem_fun_ref_t<S,T>::operator()(T& p) const {
		return (p.*pmem)();
	}

	template<class S,class T,class A>
	inline const_mem_fun1_ref_t<S,T,A>::const_mem_fun1_ref_t(S(T::*p)(A) const)
		: pmem(p) {
	}
	template<class S,class T,class A>
	inline S const_mem_fun1_ref_t<S,T,A>::operator()(T& p,A x) const {
		return (p.*pmem)(x);
	}

	template<class Predicate>
	inline unary_negate<Predicate> not1(const Predicate& pred) {
		return unary_negate<Predicate>(pred);
	}
	template<class Predicate>
	inline binary_negate<Predicate> not2(const Predicate& pred) {
		return binary_negate<Predicate>(pred);
	}

	template<class Operation,class T>
	inline binder1st<Operation> bind1st(const Operation& op,const T& x) {
		return binder1st<Operation>(op,typename Operation::first_argument_type(x));
	}
	template<class Operation,class T>
	inline binder2nd<Operation> bind2nd(const Operation& op,const T& x) {
		return binder2nd<Operation>(op,typename Operation::second_argument_type(x));
	}

	template<class Arg,class Result>
	inline pointer_to_unary_function<Arg,Result> ptr_fun(Result(*f)(Arg)) {
		return pointer_to_unary_function<Arg,Result>(f);
	}
	template<class Arg1,class Arg2,class Result>
	inline pointer_to_binary_function<Arg1,Arg2,Result> ptr_fun(Result(*f)(Arg1,Arg2)) {
		return pointer_to_binary_function<Arg1,Arg2,Result>(f);
	}
	template<class S,class T>
	inline mem_fun_t<S,T> mem_fun(S(T::*f)()) {
		return mem_fun_t<S,T> (f);
	}
	template<class S,class T,class A>
	inline mem_fun1_t<S,T,A> mem_fun(S(T::*f)(A)) {
		return mem_fun1_t<S,T,A> (f);
	}
	template<class S,class T>
	inline const_mem_fun_t<S,T> mem_fun(S(T::*f)() const) {
		return const_mem_fun_t<S,T> (f);
	}
	template<class S,class T,class A>
	inline const_mem_fun1_t<S,T,A> mem_fun(S(T::*f)(A) const) {
		return const_mem_fun1_t<S,T,A> (f);
	}
	template<class S,class T>
	inline mem_fun_ref_t<S,T> mem_fun_ref(S(T::*f)()) {
		return mem_fun_ref_t<S,T> (f);
	}
	template<class S,class T,class A>
	inline mem_fun1_ref_t<S,T,A> mem_fun_ref(S(T::*f)(A)) {
		return mem_fun1_ref_t<S,T,A> (f);
	}
	template<class S,class T>
	inline const_mem_fun_ref_t<S,T> mem_fun_ref(S(T::*f)() const) {
		return const_mem_fun_ref_t<S,T> (f);
	}
	template<class S,class T,class A>
	inline const_mem_fun1_ref_t<S,T,A> mem_fun_ref(S(T::*f)(A) const) {
		return const_mem_fun1_ref_t<S,T,A> (f);
	}
}
