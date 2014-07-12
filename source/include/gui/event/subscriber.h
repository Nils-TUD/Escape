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

#include <sys/common.h>
#include <assert.h>
#include <functor.h>

namespace gui {
	// forward declarations
	template<typename... Args>
	class Sender;

	/**
	 * The base class for all receivers. We use polymorphism here to allow different kind of
	 * receivers for the same sender.
	 */
	template<typename Result,typename... Args>
	class Receiver {
		template<typename... Args2>
		friend class Sender;

	public:
		explicit Receiver() : _next() {
		}
		virtual ~Receiver() {
		}

		virtual Result operator()(Args... arg1) = 0;

	private:
		Receiver<Result,Args...> *_next;
	};

	/**
	 * Creates a function-receiver
	 *
	 * @param cb the callback
	 * @return the receiver
	 */
	template<typename... Args>
	inline Receiver<void,Args...> *func_recv(void (*cb)(Args...)) {
		return new std::detail::FunFunctor<Receiver,void,Args...>(cb);
	}

	/**
	 * Creates a member-function-receiver
	 *
	 * @param obj the object
	 * @param cb the callback
	 * @return the receiver
	 */
	template<typename Cls,typename... Args>
	inline Receiver<void,Args...> *mem_recv(Cls *obj,void (Cls::*cb)(Args...)) {
		return new std::detail::MemFunctor<Receiver,Cls,void,Args...>(obj,cb);
	}

	/**
	 * Creates a function receiver with one argument bound to it.
	 *
	 * @param arg1 the value to bind
	 * @param cb the callback
	 * @return the receiver
	 */
	template<typename T,typename... Args>
	inline Receiver<void,Args...> *bind1_func_recv(T arg1,void (*cb)(T,Args...)) {
		return new std::detail::Bind1Functor<Receiver,T,void,Args...>(arg1,cb);
	}
	template<typename T,typename... Args>
	inline Receiver<void,Args...> *bind1_func_recv(T& arg1,void (*cb)(T&,Args...)) {
		return new std::detail::Bind1Functor<Receiver,T&,void,Args...>(arg1,cb);
	}

	/**
	 * Creates a member function receiver with one argument bound to it.
	 *
	 * @param arg1 the value to bind
	 * @param obj the object
	 * @param cb the callback
	 * @return the receiver
	 */
	template<typename T,typename Cls,typename... Args>
	inline Receiver<void,Args...> *bind1_mem_recv(
			T arg1,Cls *obj,void (Cls::*cb)(T,Args...)) {
		return new std::detail::Bind1MemFunctor<Receiver,T,Cls,void,Args...>(arg1,obj,cb);
	}
	template<typename T,typename Cls,typename... Args>
	inline Receiver<void,Args...> *bind1_mem_recv(
			T& arg1,Cls *obj,void (Cls::*cb)(T&,Args...)) {
		return new std::detail::Bind1MemFunctor<Receiver,T&,Cls,void,Args...>(arg1,obj,cb);
	}
	template<typename T,typename Cls,typename... Args>
	inline Receiver<void,Args...> *bind1_mem_recv(
			T arg1,const Cls *obj,void (Cls::*cb)(T,Args...) const) {
		return new std::detail::Bind1MemFunctor<Receiver,T,const Cls,void,Args...>(arg1,obj,cb);
	}
	template<typename T,typename Cls,typename... Args>
	inline Receiver<void,Args...> *bind1_mem_recv(
			T& arg1,const Cls *obj,void (Cls::*cb)(T&,Args...) const) {
		return new std::detail::Bind1MemFunctor<Receiver,T&,const Cls,void,Args...>(arg1,obj,cb);
	}

	/**
	 * Convenience wrapper for subscribe and unsubscribe that works in RAII way.
	 */
	template<typename... Args>
	class Subscription {
	public:
		typedef typename Sender<Args...>::sub_type sub_type;

		explicit Subscription(Sender<Args...> &sender,sub_type sub)
			: _sender(sender), _sub(_sender.subscribe(sub)) {
		}
		~Subscription() {
			_sender.unsubscribe(_sub);
		}

	private:
		Sender<Args...> &_sender;
		sub_type _sub;
	};

	/**
	 * Manages a list of receives, allows subscriptions and event sending.
	 */
	template<typename... Args>
	class Sender {
	public:
		typedef Receiver<void,Args...> *sub_type;
		typedef Subscription<Args...> subscr_type;

		explicit Sender() : _list() {
		}

		/**
		 * Subscribes <recv> to this sender to receive events.
		 *
		 * @param recv the receiver
		 * @return the id for unsubscribe()
		 */
		sub_type subscribe(sub_type recv) {
			sub_type p = nullptr;
			sub_type s = _list;
			while(s != nullptr) {
				p = s;
				s = s->_next;
			}
			if(p)
				p->_next = recv;
			else
				_list = recv;
			recv->_next = nullptr;
			return recv;
		}
		/**
		 * Unsubscribes <recv> from this sender
		 *
		 * @param recv the receiver (see subscribe())
		 */
		void unsubscribe(sub_type recv) {
			sub_type p = nullptr;
			sub_type s = _list;
			while(s != nullptr && s != recv) {
				p = s;
				s = s->_next;
			}
			assert(s == recv);
			if(p)
				p->_next = s->_next;
			else
				_list = s->_next;
			delete recv;
		}

		/**
		 * Sends the event, denoted by the arguments, to all receivers
		 *
		 * @param args the arguments to send
		 */
		void send(Args... args) {
			for(sub_type s = _list; s != nullptr; s = s->_next)
				(*s)(args...);
		}

	private:
		sub_type _list;
	};
}
