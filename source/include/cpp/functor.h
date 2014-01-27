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

namespace std {
	/**
	 * The default base class for functors
	 */
	template<typename Result,typename... Args>
	class Functor {
	public:
		explicit Functor() {
		}
		virtual ~Functor() {
		}

		virtual Result operator()(Args... arg1) = 0;
	};

	namespace detail {
		/**
		 * The functor for lambda functions
		 */
		template<
			template<typename Result2,typename... Args2> class Base,
			typename Lambda,
			typename Result
		>
		class LambdaFunctor : public Base<Result> {
		public:
			explicit LambdaFunctor(Lambda cb) : Base<Result>(), _cb(cb) {
			}

			virtual Result operator()() {
				return _cb();
			}

		private:
			Lambda _cb;
		};

		/**
		 * The functor for plain functions
		 */
		template<
			template<typename Result2,typename... Args2> class Base,
			typename Result,
			typename... Args
		>
		class FunFunctor : public Base<Result,Args...> {
		public:
			typedef Result (*callback_type)(Args...);

			explicit FunFunctor(callback_type cb)
				: Base<Result,Args...>(), _cb(cb) {
			}

			virtual Result operator()(Args... args) {
				return _cb(args...);
			}

		private:
			callback_type _cb;
		};

		/**
		 * The functor for member functions
		 */
		template<
			template<typename Result2,typename... Args2> class Base,
			class Cls,
			typename Result,
			typename... Args
		>
		class MemFunctor : public Base<Result,Args...> {
		public:
			typedef Result (Cls::*callback_type)(Args...);

			explicit MemFunctor(Cls *obj,callback_type cb)
				: Base<Result,Args...>(), _obj(obj), _cb(cb) {
			}

			virtual Result operator()(Args... args) {
				return (_obj->*_cb)(args...);
			}

		private:
			Cls *_obj;
			callback_type _cb;
		};

		/**
		 * The functor to bind an arbitrary value to a function as first argument.
		 */
		template<
			template<typename Result2,typename... Args2> class Base,
			typename T,
			typename Result,
			typename... Args
		>
		class Bind1Functor : public Base<Result,Args...> {
		public:
			typedef Result (*callback_type)(T,Args...);

			explicit Bind1Functor(T arg1,callback_type cb)
				: Base<Result,Args...>(), _arg1(arg1), _cb(cb) {
			}

			virtual Result operator()(Args... args) {
				return _cb(_arg1,args...);
			}

		private:
			T _arg1;
			callback_type _cb;
		};

		/**
		 * The functor to bind an arbitrary value to a member function as first argument.
		 */
		template<
			template<typename Result2,typename... Args2> class Base,
			typename T,
			class Cls,
			typename Result,
			typename... Args
		>
		class Bind1MemFunctor : public Base<Result,Args...> {
		public:
			typedef Result (Cls::*callback_type)(T,Args...);

			explicit Bind1MemFunctor(T arg1,Cls *obj,callback_type cb)
				: Base<Result,Args...>(), _arg1(arg1), _obj(obj), _cb(cb) {
			}

			virtual Result operator()(Args... args) {
				return (_obj->*_cb)(_arg1,args...);
			}

		private:
			T _arg1;
			Cls *_obj;
			callback_type _cb;
		};

		/**
		 * Template specialization for const member functions
		 */
		template<
			template<typename Result2,typename... Args2> class Base,
			typename T,
			class Cls,
			typename Result,
			typename... Args
		>
		class Bind1MemFunctor<Base,T,const Cls,Result,Args...> : public Base<Result,Args...> {
		public:
			typedef Result (Cls::*callback_type)(T,Args...) const;

			explicit Bind1MemFunctor(T arg1,const Cls *obj,callback_type cb)
				: Base<Result,Args...>(), _arg1(arg1), _obj(obj), _cb(cb) {
			}

			virtual Result operator()(Args... args) {
				return (_obj->*_cb)(_arg1,args...);
			}

		private:
			T _arg1;
			const Cls *_obj;
			callback_type _cb;
		};
	}

	/**
	 * Creates a functor for a lambda function. These don't have arguments.
	 *
	 * @param cb the callback
	 * @return the functor
	 */
	// the problem is that we can't find out the return-type with arguments because we would need
	// to pass them to decltype. thus, we can't use this function for all types of functions, but
	// just for lambda functions, which don't have arguments.
	template<typename Lambda>
	inline auto make_lambda(Lambda cb) -> Functor<decltype(cb())> * {
		return new detail::LambdaFunctor<Functor,Lambda,decltype(cb())>(cb);
	}

	/**
	 * Creates a functor for a plain function
	 *
	 * @param cb the callback
	 * @return the functor
	 */
	template<typename Result,typename... Args>
	inline Functor<Result,Args...> *make_fun(Result (*cb)(Args...)) {
		return new detail::FunFunctor<Functor,Result,Args...>(cb);
	}

	/**
	 * Creates a functor for a member function
	 *
	 * @param obj the object
	 * @param cb the callback
	 * @return the functor
	 */
	template<typename Cls,typename Result,typename... Args>
	inline Functor<Result,Args...> *make_memfun(Cls *obj,Result (Cls::*cb)(Args...)) {
		return new detail::MemFunctor<Functor,Cls,Result,Args...>(obj,cb);
	}

	/**
	 * Creates a functor for a plain function with one argument bound to it.
	 *
	 * @param arg1 the value to bind
	 * @param cb the callback
	 * @return the functor
	 */
	template<typename T,typename Result,typename... Args>
	inline Functor<Result,Args...> *make_bind1_fun(T arg1,Result (*cb)(T,Args...)) {
		return new detail::Bind1Functor<Functor,T,Result,Args...>(arg1,cb);
	}
	template<typename T,typename Result,typename... Args>
	inline Functor<Result,Args...> *make_bind1_fun(T& arg1,Result (*cb)(T&,Args...)) {
		return new detail::Bind1Functor<Functor,T&,Result,Args...>(arg1,cb);
	}

	/**
	 * Creates a functor for a member function with one argument bound to it.
	 *
	 * @param arg1 the value to bind
	 * @param obj the object
	 * @param cb the callback
	 * @return the functor
	 */
	template<typename T,typename Cls,typename Result,typename... Args>
	inline Functor<Result,Args...> *make_bind1_memfun(
			T arg1,Cls *obj,Result (Cls::*cb)(T,Args...)) {
		return new detail::Bind1MemFunctor<Functor,T,Cls,Result,Args...>(arg1,obj,cb);
	}
	template<typename T,typename Cls,typename Result,typename... Args>
	inline Functor<Result,Args...> *make_bind1_memfun(
			T& arg1,Cls *obj,Result (Cls::*cb)(T&,Args...)) {
		return new detail::Bind1MemFunctor<Functor,T&,Cls,Result,Args...>(arg1,obj,cb);
	}
	template<typename T,typename Cls,typename Result,typename... Args>
	inline Functor<Result,Args...> *make_bind1_memfun(
			T arg1,const Cls *obj,Result (Cls::*cb)(T,Args...) const) {
		return new detail::Bind1MemFunctor<Functor,T,const Cls,Result,Args...>(arg1,obj,cb);
	}
	template<typename T,typename Cls,typename Result,typename... Args>
	inline Functor<Result,Args...> *make_bind1_memfun(
			T& arg1,const Cls *obj,Result (Cls::*cb)(T&,Args...) const) {
		return new detail::Bind1MemFunctor<Functor,T&,const Cls,Result,Args...>(arg1,obj,cb);
	}
}
