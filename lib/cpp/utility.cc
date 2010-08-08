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
	template<typename T1,typename T2>
	pair<T1,T2>::pair()
		: first(T1()), second(T2()) {
	}
	template<typename T1,typename T2>
	pair<T1,T2>::pair(const T1& x,const T2& y)
		: first(x), second(y) {
	}
	template<typename T1,typename T2>
	template<typename U,typename V>
	pair<T1,T2>::pair(const pair<U,V> &p)
		: first(p.first), second(p.second) {
	}

	template<typename T1,typename T2>
	inline pair<T1,T2>& pair<T1,T2>::operator=(const pair<T1,T2>& p2) {
		first = p2.first;
		second = p2.second;
		return (*this);
	}

	template<class T1,class T2>
	pair<T1,T2> make_pair(T1 x,T2 y) {
		return pair<T1,T2>(x,y);
	}

	namespace rel_ops {
		template<class T>
		bool operator!=(const T& x,const T& y) {
			return !(x == y);
		}
		template<class T>
		bool operator>(const T& x,const T& y) {
			return y < x;
		}
		template<class T>
		bool operator<=(const T& x,const T& y) {
			return !(y < x);
		}
		template<class T>
		bool operator>=(const T& x,const T& y) {
			return !(x < y);
		}
	}
}
