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

#include <esc/common.h>
#include <assert.h>

namespace gui {
	// forward declarations
	template<typename... ARGS>
	class Receiver;
	template<typename... ARGS>
	class Sender;

	template<typename... ARGS>
	Receiver<ARGS...> *func_recv(void (*cb)(ARGS...));
	template<typename CLS,typename... ARGS>
	Receiver<ARGS...> *mem_recv(CLS *obj,void (CLS::*cb)(ARGS...));
	template<typename T,typename... ARGS>
	Receiver<ARGS...> *bind1_func_recv(T arg1,void (*cb)(T,ARGS...));
	template<typename T,typename... ARGS>
	Receiver<ARGS...> *bind1_func_recv(T& arg1,void (*cb)(T&,ARGS...));
	template<typename T,typename CLS,typename... ARGS>
	Receiver<ARGS...> *bind1_mem_recv(T arg1,CLS *obj,void (CLS::*cb)(T,ARGS...));
	template<typename T,typename CLS,typename... ARGS>
	Receiver<ARGS...> *bind1_mem_recv(T& arg1,CLS *obj,void (CLS::*cb)(T&,ARGS...));
	template<typename T,typename CLS,typename... ARGS>
	Receiver<ARGS...> *bind1_mem_recv(T arg1,const CLS *obj,void (CLS::*cb)(T,ARGS...) const);
	template<typename T,typename CLS,typename... ARGS>
	Receiver<ARGS...> *bind1_mem_recv(T& arg1,const CLS *obj,void (CLS::*cb)(T&,ARGS...) const);

	/**
	 * The base class for all receivers. We use polymorphism here to allow different kind of
	 * receivers for the same sender.
	 */
	template<typename... ARGS>
	class Receiver {
		template<typename... RARGS>
		friend class Sender;

	public:
		explicit Receiver() : _next() {
		}
		virtual ~Receiver() {
		}

	private:
		virtual void send(ARGS... arg1) = 0;

		Receiver<ARGS...> *_next;
	};

	/**
	 * The receiver for plain functions
	 */
	template<typename... ARGS>
	class FuncReceiver : public Receiver<ARGS...> {
		template<typename... RARGS>
		friend Receiver<RARGS...> *func_recv(void (*cb)(RARGS...));

	    typedef void (*callback_type)(ARGS...);

		explicit FuncReceiver(callback_type cb) : Receiver<ARGS...>(), _cb(cb) {
		}

		virtual void send(ARGS... args) {
			_cb(args...);
		}

		callback_type _cb;
	};

	/**
	 * The receiver for member functions
	 */
	template<class CLS,typename... ARGS>
	class MemberReceiver : public Receiver<ARGS...> {
		template<typename RCLS,typename... RARGS>
		friend Receiver<RARGS...> *mem_recv(RCLS *obj,void (RCLS::*cb)(RARGS...));

	    typedef void (CLS::*callback_type)(ARGS...);

		explicit MemberReceiver(CLS *obj,callback_type cb) : Receiver<ARGS...>(), _obj(obj), _cb(cb) {
		}

		virtual void send(ARGS... args) {
			(_obj->*_cb)(args...);
		}

		CLS *_obj;
		callback_type _cb;
	};

	/**
	 * The receiver to bind an arbitrary value to a function as first argument.
	 */
	template<typename T,typename... ARGS>
	class Bind1FuncReceiver : public Receiver<ARGS...> {
		template<typename T1,typename... RARGS>
		friend Receiver<RARGS...> *bind1_func_recv(T1 val,void (*cb)(T1,RARGS...));
		template<typename T1,typename... RARGS>
		friend Receiver<RARGS...> *bind1_func_recv(T1& val,void (*cb)(T1&,RARGS...));

	    typedef void (*callback_type)(T,ARGS...);

		explicit Bind1FuncReceiver(T arg1,callback_type cb) : Receiver<ARGS...>(), _arg1(arg1), _cb(cb) {
		}

		virtual void send(ARGS... args) {
			_cb(_arg1,args...);
		}

		T _arg1;
		callback_type _cb;
	};

	/**
	 * The receiver to bind an arbitrary value to a member function as first argument.
	 */
	template<typename T,class CLS,typename... ARGS>
	class Bind1MemberReceiver : public Receiver<ARGS...> {
		template<typename T1,typename RCLS,typename... RARGS>
		friend Receiver<RARGS...> *bind1_mem_recv(T1 val,RCLS *obj,void (RCLS::*cb)(T1,RARGS...));
		template<typename T1,typename RCLS,typename... RARGS>
		friend Receiver<RARGS...> *bind1_mem_recv(T1& val,RCLS *obj,void (RCLS::*cb)(T1&,RARGS...));

	    typedef void (CLS::*callback_type)(T,ARGS...);

		explicit Bind1MemberReceiver(T arg1,CLS *obj,callback_type cb)
			: Receiver<ARGS...>(), _arg1(arg1), _obj(obj), _cb(cb) {
		}

		virtual void send(ARGS... args) {
			(_obj->*_cb)(_arg1,args...);
		}

		T _arg1;
		CLS *_obj;
		callback_type _cb;
	};
	/**
	 * Template specialization for const member functions
	 */
	template<typename T,class CLS,typename... ARGS>
	class Bind1MemberReceiver<T,const CLS,ARGS...> : public Receiver<ARGS...> {
		template<typename T1,typename RCLS,typename... RARGS>
		friend Receiver<RARGS...> *bind1_mem_recv(
				T1 val,const RCLS *obj,void (RCLS::*cb)(T1,RARGS...) const);
		template<typename T1,typename RCLS,typename... RARGS>
		friend Receiver<RARGS...> *bind1_mem_recv(
				T1& val,const RCLS *obj,void (RCLS::*cb)(T1&,RARGS...) const);

	    typedef void (CLS::*callback_type)(T,ARGS...) const;

		explicit Bind1MemberReceiver(T arg1,const CLS *obj,callback_type cb)
			: Receiver<ARGS...>(), _arg1(arg1), _obj(obj), _cb(cb) {
		}

		virtual void send(ARGS... args) {
			(_obj->*_cb)(_arg1,args...);
		}

		T _arg1;
		const CLS *_obj;
		callback_type _cb;
	};

	/**
	 * Creates a function-receiver
	 *
	 * @param cb the callback
	 * @return the receiver
	 */
	template<typename... ARGS>
	inline Receiver<ARGS...> *func_recv(void (*cb)(ARGS...)) {
		return new FuncReceiver<ARGS...>(cb);
	}

	/**
	 * Creates a member-function-receiver
	 *
	 * @param obj the object
	 * @param cb the callback
	 * @return the receiver
	 */
	template<typename CLS,typename... ARGS>
	inline Receiver<ARGS...> *mem_recv(CLS *obj,void (CLS::*cb)(ARGS...)) {
		return new MemberReceiver<CLS,ARGS...>(obj,cb);
	}

	/**
	 * Creates a function receiver with one argument bound to it.
	 *
	 * @param arg1 the value to bind
	 * @param cb the callback
	 * @return the receiver
	 */
	template<typename T,typename... ARGS>
	inline Receiver<ARGS...> *bind1_func_recv(T arg1,void (*cb)(T,ARGS...)) {
		return new Bind1FuncReceiver<T,ARGS...>(arg1,cb);
	}
	template<typename T,typename... ARGS>
	inline Receiver<ARGS...> *bind1_func_recv(T& arg1,void (*cb)(T&,ARGS...)) {
		return new Bind1FuncReceiver<T&,ARGS...>(arg1,cb);
	}

	/**
	 * Creates a member function receiver with one argument bound to it.
	 *
	 * @param arg1 the value to bind
	 * @param obj the object
	 * @param cb the callback
	 * @return the receiver
	 */
	template<typename T,typename CLS,typename... ARGS>
	inline Receiver<ARGS...> *bind1_mem_recv(T arg1,CLS *obj,void (CLS::*cb)(T,ARGS...)) {
		return new Bind1MemberReceiver<T,CLS,ARGS...>(arg1,obj,cb);
	}
	template<typename T,typename CLS,typename... ARGS>
	inline Receiver<ARGS...> *bind1_mem_recv(T& arg1,CLS *obj,void (CLS::*cb)(T&,ARGS...)) {
		return new Bind1MemberReceiver<T&,CLS,ARGS...>(arg1,obj,cb);
	}
	template<typename T,typename CLS,typename... ARGS>
	inline Receiver<ARGS...> *bind1_mem_recv(T arg1,const CLS *obj,void (CLS::*cb)(T,ARGS...) const) {
		return new Bind1MemberReceiver<T,const CLS,ARGS...>(arg1,obj,cb);
	}
	template<typename T,typename CLS,typename... ARGS>
	inline Receiver<ARGS...> *bind1_mem_recv(T& arg1,const CLS *obj,void (CLS::*cb)(T&,ARGS...) const) {
		return new Bind1MemberReceiver<T&,const CLS,ARGS...>(arg1,obj,cb);
	}

	/**
	 * Convenience wrapper for subscribe and unsubscribe that works in RAII way.
	 */
	template<typename... ARGS>
	class Subscription {
	public:
		typedef typename Sender<ARGS...>::sub_type sub_type;

		explicit Subscription(Sender<ARGS...> &sender,sub_type sub)
			: _sender(sender), _sub(_sender.subscribe(sub)) {
		}
		~Subscription() {
			_sender.unsubscribe(_sub);
		}

	private:
		Sender<ARGS...> &_sender;
		sub_type _sub;
	};

	/**
	 * Manages a list of receives, allows subscriptions and event sending.
	 */
	template<typename... ARGS>
	class Sender {
	public:
		typedef Receiver<ARGS...> *sub_type;
		typedef Subscription<ARGS...> subscr_type;

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
		void send(ARGS... args) {
			for(sub_type s = _list; s != nullptr; s = s->_next)
				s->send(args...);
		}

	private:
		sub_type _list;
	};
}
